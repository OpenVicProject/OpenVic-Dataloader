#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/_detail/config.hpp>
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/encoding.hpp>

#include "detail/InternalConcepts.hpp"

// Grammar Definitions //
namespace ovdl::csv::grammar {
	struct ParseOptions {
		/// @brief Separator character
		char SepChar;
		/// @brief Determines whether StringValue is supported
		bool SupportStrings;
		/// @brief Paradox-style localization escape characters
		/// @note Is ignored if SupportStrings is true
		char EscapeChar;
	};

	struct ConvertErrorHandler {
		static constexpr void on_invalid_character(detail::IsStateType auto& state, auto reader) {
			state.logger().warning("invalid character value '{}' found", static_cast<int>(reader.peek())) //
				.primary(BasicNodeLocation { reader.position() }, "here")
				.finish();
		}
	};

	constexpr auto utf_character = lexy::dsl::unicode::character;
	constexpr auto utf_control = lexy::dsl::unicode::control;

	constexpr auto escaped_symbols = lexy::symbol_table<char> //
										 .map<'"'>('"')
										 .map<'\''>('\'')
										 .map<'\\'>('\\')
										 .map<'/'>('/')
										 .map<'b'>('\b')
										 .map<'f'>('\f')
										 .map<'n'>('\n')
										 .map<'r'>('\r')
										 .map<'t'>('\t');

	constexpr auto escaped_quote = lexy::symbol_table<char> //
									   .map<'"'>('"');

	constexpr auto escaped_newline = lexy::symbol_table<char> //
										 .map<'n'>('\n');

	template<ParseOptions Options>
	struct CsvGrammar {
		struct StringValue : lexy::token_production {
			static constexpr auto rule = [] {
				auto quote = lexy::dsl::lit_c<'"'>;
				auto c = utf_character - utf_control;
				auto back_escape = lexy::dsl::backslash_escape.symbol<escaped_symbols>();
				auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>).template symbol<escaped_quote>();

				return lexy::dsl::delimited(quote, lexy::dsl::not_followed_by(quote, quote))(c, back_escape, quote_escape);
			}();

			static constexpr auto value = lexy::as_string<std::string>;
		};

		struct PlainValue : lexy::token_production {
			template<auto character>
			static constexpr auto _escape_check = character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline);

			struct Backslash {
				static constexpr auto rule = LEXY_LIT("\\n");
				static constexpr auto value = lexy::constant('\n');
			};

			static constexpr auto rule = [] {
				if constexpr (Options.SupportStrings) {
					return lexy::dsl::identifier(utf_character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline));
				} else {
					constexpr auto backslash = lexy::dsl::lit_b<'\\'>;

					constexpr auto escape_check_char = _escape_check<utf_character>;
					constexpr auto escape_rule = lexy::dsl::p<Backslash>;

					return lexy::dsl::list(
						lexy::dsl::identifier(escape_check_char - backslash) |
						escape_rule |
						lexy::dsl::capture(escape_check_char) //
					);
				}
			}();

			static constexpr auto value = lexy::as_string<std::string>;
		};

		struct Value {
			static constexpr auto rule = [] {
				if constexpr (Options.SupportStrings) {
					return lexy::dsl::p<StringValue> | lexy::dsl::p<PlainValue>;
				} else {
					return lexy::dsl::p<PlainValue>;
				}
			}();
			static constexpr auto value = lexy::forward<std::string>;
		};

		struct SepConst {
			static constexpr auto rule = lexy::dsl::lit_b<Options.SepChar>;
			static constexpr auto value = lexy::constant(1);
		};

		struct Separator {
			static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SepConst>);
			static constexpr auto value = lexy::count;
		};

		struct LineEnd {
			static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<Value>, lexy::dsl::trailing_sep(lexy::dsl::p<Separator>));
			static constexpr auto value = lexy::fold_inplace<ovdl::csv::LineObject>(
				std::initializer_list<ovdl::csv::LineObject::value_type> {},
				[](ovdl::csv::LineObject& result, std::size_t&& arg) {
					// Count separators, adds to previous value, making it a position
					using position_type = ovdl::csv::LineObject::position_type;
					result.emplace_back(static_cast<position_type>(arg + result.back().first), "");
				},
				[](ovdl::csv::LineObject& result, std::string&& arg) {
					if (result.empty()) {
						result.emplace_back(0u, LEXY_MOV(arg));
					} else {
						auto& [pos, value] = result.back();
						value = LEXY_MOV(arg);
					}
				});
		};

		struct Line {
			static constexpr auto suffix_setter(ovdl::csv::LineObject& line) {
				auto& [position, value] = line.back();
				if (value.empty()) {
					line.set_suffix_end(position);
					line.pop_back();
				} else {
					line.set_suffix_end(position + 1);
				}
			};

			static constexpr auto rule = lexy::dsl::p<LineEnd> | lexy::dsl::p<Separator> >> lexy::dsl::opt(lexy::dsl::p<LineEnd>);
			static constexpr auto value =
				lexy::callback<ovdl::csv::LineObject>(
					[](ovdl::csv::LineObject&& line) {
						suffix_setter(line);
						return LEXY_MOV(line);
					},
					[](std::size_t prefix_count, ovdl::csv::LineObject&& line) {
						line.set_prefix_end(prefix_count);
						// position needs to be adjusted to prefix
						for (auto& [position, value] : line) {
							position += prefix_count;
						}
						suffix_setter(line);
						return LEXY_MOV(line);
					},
					[](std::size_t suffix_count, lexy::nullopt = {}) {
						return ovdl::csv::LineObject(0, {}, suffix_count + 1);
					});
		};
	};

	template<ParseOptions Options>
	struct File {
		static constexpr auto rule = lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<typename CsvGrammar<Options>::Line> | lexy::dsl::newline);

		static constexpr auto value = lexy::as_list<std::vector<ovdl::csv::LineObject>>;
	};

	using CommaFile = File<ParseOptions { ',', false, '$' }>;
	using ColonFile = File<ParseOptions { ':', false, '$' }>;
	using SemiColonFile = File<ParseOptions { ';', false, '$' }>;
	using TabFile = File<ParseOptions { '\t', false, '$' }>;
	using BarFile = File<ParseOptions { '|', false, '$' }>;

	namespace strings {
		using CommaFile = File<ParseOptions { ',', true, '$' }>;
		using ColonFile = File<ParseOptions { ':', true, '$' }>;
		using SemiColonFile = File<ParseOptions { ';', true, '$' }>;
		using TabFile = File<ParseOptions { '\t', true, '$' }>;
		using BarFile = File<ParseOptions { '|', true, '$' }>;
	}
}