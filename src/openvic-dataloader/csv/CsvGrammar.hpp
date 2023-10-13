#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "detail/LexyLitRange.hpp"

// Grammar Definitions //
namespace ovdl::csv::grammar {
	template<typename T>
	concept ParseChars = requires() {
		{ T::character };
		{ T::control };
	};

	template<ParseChars T>
	struct ParseOptions {
		/// @brief Seperator character
		char SepChar;
		/// @brief Determines whether StringValue is supported
		bool SupportStrings;
		/// @brief Paradox-style localization escape characters
		/// @note Is ignored if SupportStrings is true
		char EscapeChar;

		static constexpr auto parse_chars = T {};
		static constexpr auto character = parse_chars.character;
		static constexpr auto control = parse_chars.control;
	};

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
	struct StringValue {
		static constexpr auto rule = [] {
			// Arbitrary code points
			auto c = Options.character - Options.control;

			auto back_escape = lexy::dsl::backslash_escape //
								   .symbol<escaped_symbols>();

			auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
									.template symbol<escaped_quote>();

			return lexy::dsl::delimited(lexy::dsl::lit_c<'"'>, lexy::dsl::not_followed_by(lexy::dsl::lit_c<'"'>, lexy::dsl::lit_c<'"'>))(c, back_escape, quote_escape);
		}();

		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<ParseOptions Options>
	struct PlainValue {
		static constexpr auto rule = [] {
			if constexpr (Options.SupportStrings) {
				return lexy::dsl::identifier(Options.character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline));
			} else {
				auto escape_check_char = Options.character - (lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline);
				auto id_check_char = escape_check_char - lexy::dsl::lit_b<'\\'>;
				auto id_segment = lexy::dsl::identifier(id_check_char);
				auto escape_segement = lexy::dsl::token(escape_check_char);
				auto escape_sym = lexy::dsl::symbol<escaped_symbols>(escape_segement);
				auto escape_rule = lexy::dsl::lit_b<'\\'> >> escape_sym;
				return lexy::dsl::list(id_segment | escape_rule);
			}
		}();
		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<ParseOptions Options>
	struct Value {
		static constexpr auto rule = [] {
			if constexpr (Options.SupportStrings) {
				return lexy::dsl::p<StringValue<Options>> | lexy::dsl::p<PlainValue<Options>>;
			} else {
				return lexy::dsl::p<PlainValue<Options>>;
			}
		}();
		static constexpr auto value = lexy::forward<std::string>;
	};

	template<ParseOptions Options>
	struct SepConst {
		static constexpr auto rule = lexy::dsl::lit_b<Options.SepChar>;
		static constexpr auto value = lexy::constant(1);
	};

	template<ParseOptions Options>
	struct Seperator {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SepConst<Options>>);
		static constexpr auto value = lexy::count;
	};

	template<ParseOptions Options>
	struct LineEnd {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<Value<Options>>, lexy::dsl::trailing_sep(lexy::dsl::p<Seperator<Options>>));
		static constexpr auto value = lexy::fold_inplace<ovdl::csv::LineObject>(
			std::initializer_list<ovdl::csv::LineObject::value_type> {},
			[](ovdl::csv::LineObject& result, auto&& arg) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::size_t>) {
					// Count seperators, adds to previous value, making it a position
					using position_type = ovdl::csv::LineObject::position_type;
					result.emplace_back(static_cast<position_type>(arg + result.back().first), "");
				} else {
					if (result.empty()) result.emplace_back(0u, LEXY_MOV(arg));
					else {
						auto& [pos, value] = result.back();
						value = arg;
					}
				}
			});
	};

	template<ParseOptions Options>
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

		static constexpr auto rule = lexy::dsl::p<LineEnd<Options>> | lexy::dsl::p<Seperator<Options>> >> lexy::dsl::opt(lexy::dsl::p<LineEnd<Options>>);
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

	template<ParseOptions Options>
	struct File {
		static constexpr auto rule =
			lexy::dsl::whitespace(lexy::dsl::newline) +
			lexy::dsl::opt(lexy::dsl::list(lexy::dsl::p<Line<Options>>, lexy::dsl::trailing_sep(lexy::dsl::eol)));

		static constexpr auto value = lexy::as_list<std::vector<ovdl::csv::LineObject>>;
	};

	template<ParseChars T>
	using CommaFile = File<ParseOptions<T> { ',', false, '$' }>;
	template<ParseChars T>
	using ColonFile = File<ParseOptions<T> { ':', false, '$' }>;
	template<ParseChars T>
	using SemiColonFile = File<ParseOptions<T> { ';', false, '$' }>;
	template<ParseChars T>
	using TabFile = File<ParseOptions<T> { '\t', false, '$' }>;
	template<ParseChars T>
	using BarFile = File<ParseOptions<T> { '|', false, '$' }>;

	namespace strings {
		template<ParseChars T>
		using CommaFile = File<ParseOptions<T> { ',', true, '$' }>;
		template<ParseChars T>
		using ColonFile = File<ParseOptions<T> { ':', true, '$' }>;
		template<ParseChars T>
		using SemiColonFile = File<ParseOptions<T> { ';', true, '$' }>;
		template<ParseChars T>
		using TabFile = File<ParseOptions<T> { '\t', true, '$' }>;
		template<ParseChars T>
		using BarFile = File<ParseOptions<T> { '|', true, '$' }>;
	}
}

namespace ovdl::csv::grammar::windows1252 {
	struct windows1252_t {
		static constexpr auto character = detail::lexydsl::make_range<0x01, 0xFF>();
		static constexpr auto control =
			lexy::dsl::ascii::control /
			lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
			lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;
	};

	using CommaFile = CommaFile<windows1252_t>;
	using ColonFile = ColonFile<windows1252_t>;
	using SemiColonFile = SemiColonFile<windows1252_t>;
	using TabFile = TabFile<windows1252_t>;
	using BarFile = BarFile<windows1252_t>;

	namespace strings {
		using CommaFile = grammar::strings::CommaFile<windows1252_t>;
		using ColonFile = grammar::strings::ColonFile<windows1252_t>;
		using SemiColonFile = grammar::strings::SemiColonFile<windows1252_t>;
		using TabFile = grammar::strings::TabFile<windows1252_t>;
		using BarFile = grammar::strings::BarFile<windows1252_t>;

	}
}

namespace ovdl::csv::grammar::utf8 {
	struct unicode_t {
		static constexpr auto character = lexy::dsl::unicode::character;
		static constexpr auto control = lexy::dsl::unicode::control;
	};

	using CommaFile = CommaFile<unicode_t>;
	using ColonFile = ColonFile<unicode_t>;
	using SemiColonFile = SemiColonFile<unicode_t>;
	using TabFile = TabFile<unicode_t>;
	using BarFile = BarFile<unicode_t>;

	namespace strings {
		using CommaFile = grammar::strings::CommaFile<unicode_t>;
		using ColonFile = grammar::strings::ColonFile<unicode_t>;
		using SemiColonFile = grammar::strings::SemiColonFile<unicode_t>;
		using TabFile = grammar::strings::TabFile<unicode_t>;
		using BarFile = grammar::strings::BarFile<unicode_t>;

	}
}