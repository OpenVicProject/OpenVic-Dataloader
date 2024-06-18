#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/_detail/config.hpp>
#include <lexy/callback.hpp>
#include <lexy/callback/string.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/ascii.hpp>
#include <lexy/dsl/option.hpp>
#include <lexy/encoding.hpp>

#include "detail/Convert.hpp"
#include "detail/InternalConcepts.hpp"
#include "detail/dsl.hpp"

// Grammar Definitions //
namespace ovdl::csv::grammar {
	struct ParseOptions {
		/// @brief Seperator character
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

	constexpr bool IsUtf8(auto encoding) {
		return std::same_as<std::decay_t<decltype(encoding)>, lexy::utf8_char_encoding>;
	}

	template<ParseOptions Options, typename String>
	constexpr auto convert_as_string = convert::convert_as_string<
		String,
		ConvertErrorHandler>;

	constexpr auto ansi_character = lexy::dsl::ascii::character / dsl::lit_b_range<0x80, 0xFF>;
	constexpr auto ansi_control =
		lexy::dsl::ascii::control /
		lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;

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

	template<ParseOptions Options>
	struct CsvGrammar {
		struct StringValue : lexy::scan_production<std::string>,
							 lexy::token_production {

			template<typename Context, typename Reader>
			static constexpr scan_result scan(lexy::rule_scanner<Context, Reader>& scanner, detail::IsFileParseState auto& state) {
				using encoding = typename Reader::encoding;

				constexpr auto rule = [] {
					// Arbitrary code points
					auto c = [] {
						if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
							return ansi_character - ansi_control;
						} else {
							return utf_character - utf_control;
						}
					}();

					auto back_escape = lexy::dsl::backslash_escape //
										   .symbol<escaped_symbols>();

					auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
											.template symbol<escaped_quote>();

					return lexy::dsl::delimited(lexy::dsl::lit_c<'"'>, lexy::dsl::not_followed_by(lexy::dsl::lit_c<'"'>, lexy::dsl::lit_c<'"'>))(c, back_escape, quote_escape);
				}();

				lexy::scan_result<std::string> str_result = scanner.template parse<std::string>(rule);
				if (!scanner || !str_result)
					return lexy::scan_failed;
				return str_result.value();
			}

			static constexpr auto rule = lexy::dsl::peek(lexy::dsl::lit_c<'"'>) >> lexy::dsl::scan;

			static constexpr auto value = convert_as_string<Options, std::string> >> lexy::forward<std::string>;
		};

		struct PlainValue : lexy::scan_production<std::string>,
							lexy::token_production {

			template<auto character>
			static constexpr auto _escape_check = character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline);

			template<typename Context, typename Reader>
			static constexpr scan_result scan(lexy::rule_scanner<Context, Reader>& scanner, detail::IsFileParseState auto& state) {
				using encoding = typename Reader::encoding;

				constexpr auto rule = [] {
					constexpr auto character = [] {
						if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
							return ansi_character;
						} else {
							return utf_character;
						}
					}();

					if constexpr (Options.SupportStrings) {
						return lexy::dsl::identifier(character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline));
					} else {
						auto escape_check_char = _escape_check<character>;
						auto id_check_char = escape_check_char - lexy::dsl::lit_b<'\\'>;
						auto id_segment = lexy::dsl::identifier(id_check_char);
						auto escape_segement = lexy::dsl::token(escape_check_char);
						auto escape_sym = lexy::dsl::symbol<escaped_symbols>(escape_segement);
						auto escape_rule = lexy::dsl::lit_b<'\\'> >> escape_sym;
						return lexy::dsl::list(id_segment | escape_rule);
					}
				}();

				if constexpr (Options.SupportStrings) {
					auto lexeme_result = scanner.template parse<lexy::lexeme<Reader>>(rule);
					if (!scanner || !lexeme_result)
						return lexy::scan_failed;
					return std::string { lexeme_result.value().begin(), lexeme_result.value().end() };
				} else {
					lexy::scan_result<std::string> str_result = scanner.template parse<std::string>(rule);
					if (!scanner || !str_result)
						return lexy::scan_failed;
					return str_result.value();
				}
			}

			static constexpr auto rule =
				dsl::peek(
					_escape_check<ansi_character>,
					_escape_check<utf_character>) >>
				lexy::dsl::scan;

			static constexpr auto value = convert_as_string<Options, std::string> >> lexy::forward<std::string>;
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

		struct Seperator {
			static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SepConst>);
			static constexpr auto value = lexy::count;
		};

		struct LineEnd {
			static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<Value>, lexy::dsl::trailing_sep(lexy::dsl::p<Seperator>));
			static constexpr auto value = lexy::fold_inplace<ovdl::csv::LineObject>(
				std::initializer_list<ovdl::csv::LineObject::value_type> {},
				[](ovdl::csv::LineObject& result, std::size_t&& arg) {
					// Count seperators, adds to previous value, making it a position
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

			static constexpr auto rule = lexy::dsl::p<LineEnd> | lexy::dsl::p<Seperator> >> lexy::dsl::opt(lexy::dsl::p<LineEnd>);
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