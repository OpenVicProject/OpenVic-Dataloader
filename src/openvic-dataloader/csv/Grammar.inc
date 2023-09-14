// This is designed to be reused in CsvGrammar.hpp multiple times
// Please ignore undeclared variable use for character and control, this is intended
// Sorry was the cleanest way I could think of to prevent code duplication

// Includes to keep file errors small
#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <fmt/format.h>

#include "detail/LexyLitRange.hpp"

struct ParseOptions {
	/// @brief Seperator character
	char SepChar;
	/// @brief Determines whether StringValue is supported
	bool SupportStrings;
	/// @brief Paradox-style localization escape characters
	/// @note Is ignored if SupportStrings is true
	char EscapeChar;
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
		auto c = character - control;

		auto back_escape = lexy::dsl::backslash_escape //
							   .symbol<escaped_symbols>();

		auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
								.symbol<escaped_quote>();

		auto quotes = lexy::dsl::delimited(lexy::dsl::lit_c<'"'>, lexy::dsl::not_followed_by(lexy::dsl::lit_c<'"'>, lexy::dsl::lit_c<'"'>));

		return quotes(c, back_escape, quote_escape);
	}();

	static constexpr auto value = lexy::as_string<std::string>;
};

template<ParseOptions Options>
struct EscapeValue {
	static constexpr auto rule = [] {
		auto id = lexy::dsl::identifier(id_head, id_tail);

		return lexy::dsl::lit_b<Options.EscapeChar> >>
			   (lexy::dsl::lit_b<Options.EscapeChar> |
				   (id >> lexy::dsl::lit_b<Options.EscapeChar>));
	}();
	static constexpr auto value =
		lexy::callback_with_state<std::string>(
			[](const auto& state, auto&& lexeme) {
				auto check = std::string_view { lexeme.data(), lexeme.size() };
				if (auto value = state.find_value(check); value != state.end())
					return std::string(value->second.data(), value->second.size());
				return fmt::format("${}$", check);
			},
			[](auto&& lexeme) {
				return fmt::format("${}$", std::string_view { lexeme.data(), lexeme.size() });
			},
			[](lexy::nullopt = {}) {
				return std::string(1, Options.EscapeChar);
			},
			[](const auto& state, lexy::nullopt = {}) {
				return std::string(1, Options.EscapeChar);
			});
};

template<ParseOptions Options>
struct PlainValue {
	static constexpr auto rule = [] {
		auto min_skip = lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline;
		if constexpr (Options.SupportStrings) {
			return lexy::dsl::identifier(character - min_skip);
		} else {
			auto escape_check_char = [=] {
				if constexpr (Options.EscapeChar != 0) {
					return character - (min_skip / lexy::dsl::lit_b<Options.EscapeChar>);
				} else {
					return character - min_skip;
				}
			}();
			auto id_check_char = escape_check_char - lexy::dsl::lit_b<'\\'>;
			auto id_segment = lexy::dsl::identifier(id_check_char);
			auto escape_segement = lexy::dsl::token(escape_check_char);
			auto escape_sym = lexy::dsl::symbol<escaped_symbols>(escape_segement);
			auto escape_rule = lexy::dsl::lit_b<'\\'> >> escape_sym;
			if constexpr (Options.EscapeChar != 0) {
				return lexy::dsl::list(lexy::dsl::p<EscapeValue<Options>> | id_segment | escape_rule);
			} else {
				return lexy::dsl::list(id_segment | escape_rule);
			}
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

	static constexpr auto rule = lexy::dsl::p<LineEnd<Options>> | lexy::dsl::p<Seperator<Options>> >> lexy::dsl::p<LineEnd<Options>>;
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
			});
};

template<ParseOptions Options>
struct File {
	static constexpr auto rule =
		lexy::dsl::whitespace(lexy::dsl::newline) +
		lexy::dsl::opt(lexy::dsl::list(lexy::dsl::p<Line<Options>>, lexy::dsl::trailing_sep(lexy::dsl::eol)));

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
