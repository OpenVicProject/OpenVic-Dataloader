#pragma once

#include <algorithm>
#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/lexeme.hpp>

#include "openvic-dataloader/csv/ValueNode.hpp"

#include "detail/LexyLitRange.hpp"

namespace ovdl::csv::grammar {
	template<typename T>
	concept ParseChars = requires() {
		{ T::character };
		{ T::control };
		{ T::xid_start_underscore };
		{ T::xid_continue };
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
			auto c = Options.parse_chars.character - Options.parse_chars.control;

			auto back_escape = lexy::dsl::backslash_escape //
								   .symbol<escaped_symbols>();

			auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
									.template symbol<escaped_quote>();

			auto quotes = lexy::dsl::delimited(lexy::dsl::lit_c<'"'>, lexy::dsl::not_followed_by(lexy::dsl::lit_c<'"'>, lexy::dsl::lit_c<'"'>));

			return quotes(c, back_escape, quote_escape);
		}();

		static constexpr auto value =
			lexy::as_string<std::string> >>
			lexy::callback<ovdl::csv::ValueNode>(
				[](auto&& string) {
					return ovdl::csv::ValueNode(LEXY_MOV(string));
				});
		;
	};

	template<ParseOptions Options>
	struct EscapeValue {
		static constexpr auto rule = [] {
			auto id = lexy::dsl::identifier(Options.parse_chars.xid_start_underscore, Options.parse_chars.xid_continue);

			return lexy::dsl::lit_b<Options.EscapeChar> >>
				   (lexy::dsl::lit_b<Options.EscapeChar> |
					   (id >> lexy::dsl::lit_b<Options.EscapeChar>));
		}();
		static constexpr auto value =
			lexy::callback<ovdl::csv::ValueNode::internal_value_type>(
				[](auto&& lexeme) {
					return ovdl::csv::ValueNode::Placeholder { std::string { lexeme.data(), lexeme.size() } };
				},
				[](lexy::nullopt = {}) {
					return std::string(1, Options.EscapeChar);
				});
	};

	template<ParseOptions Options>
	struct PlainValue {
		static constexpr auto rule = [] {
			auto min_skip = lexy::dsl::lit_b<Options.SepChar> / lexy::dsl::ascii::newline;
			if constexpr (Options.SupportStrings) {
				return lexy::dsl::identifier(Options.parse_chars.character - min_skip);
			} else {
				auto escape_check_char = [=] {
					if constexpr (Options.EscapeChar != 0) {
						return Options.parse_chars.character - (min_skip / lexy::dsl::lit_b<Options.EscapeChar>);
					} else {
						return Options.parse_chars.character - min_skip;
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
		static constexpr auto value =
			lexy::fold_inplace<std::vector<ovdl::csv::ValueNode::internal_value_type>>(
				std::initializer_list<ovdl::csv::ValueNode::internal_value_type> {},
				[](auto& list, const char& c) {
					if (list.empty()) {
						list.push_back(std::string { c, 1 });
						return;
					}
					std::visit([&](auto&& arg) {
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, std::string>) {
							arg += c;
						} else {
							list.push_back(std::string { c, 1 });
						}
					},
						list.back());
				},
				[]<typename Reader>(auto& list, lexy::lexeme<Reader> lexeme) {
					if (list.empty()) {
						list.push_back(std::string { lexeme.data(), lexeme.size() });
						return;
					}
					std::visit([&](auto&& arg) {
						using T = std::decay_t<decltype(arg)>;
						if constexpr (std::is_same_v<T, std::string>) {
							arg += std::string_view { lexeme.data(), lexeme.size() };
						} else {
							list.push_back(std::string { lexeme.data(), lexeme.size() });
						}
					},
						list.back());
				},
				[](auto& list, ovdl::csv::ValueNode::internal_value_type&& arg) {
					if (list.empty()) {
						list.push_back(LEXY_MOV(arg));
						return;
					}
					using T = std::decay_t<decltype(arg)>;
					std::visit([&](auto&& list_arg) {
						using list_T = std::decay_t<decltype(list_arg)>;
						if constexpr (std::is_same_v<T, std::string> && std::is_same_v<list_T, std::string>) {
							list_arg += LEXY_MOV(arg);
						} else {
							list.push_back(LEXY_MOV(arg));
						}
					},
						list.back());
				}) >>
			lexy::callback<ovdl::csv::ValueNode>(
				[](std::vector<ovdl::csv::ValueNode::internal_value_type>&& list) {
					return ovdl::csv::ValueNode(LEXY_MOV(list));
				},
				[](auto&& lexeme) {
					ovdl::csv::ValueNode result;
					result.set_as_list(std::string { lexeme.data(), lexeme.size() });
					return result;
				});
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
		static constexpr auto value = lexy::forward<ovdl::csv::ValueNode>;
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
					result.emplace_back("", static_cast<position_type>(arg + result.back().get_position()));
				} else {
					if (result.empty()) result.push_back(LEXY_MOV(arg));
					else {
						auto& object = result.back();
						object = LEXY_MOV(arg);
					}
				}
			});
	};

	template<ParseOptions Options>
	struct Line {

		static constexpr auto suffix_setter(ovdl::csv::LineObject& line) {
			auto& object = line.back();
			if (object.list_is_empty()) {
				line.set_suffix_end(object.get_position());
				line.pop_back();
			} else {
				line.set_suffix_end(object.get_position() + 1);
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
					for (auto& object : line) {
						object.set_position(object.get_position() + prefix_count);
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
}

// Grammar Definitions //
namespace ovdl::csv::grammar::windows1252 {
	struct windows1252_t {
		static constexpr auto character = detail::lexydsl::make_range<0x01, 0xFF>();
		static constexpr auto control =
			lexy::dsl::ascii::control /
			lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
			lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;
		static constexpr auto xid_start_underscore = lexy::dsl::ascii::alpha_underscore;
		static constexpr auto xid_continue = lexy::dsl::ascii::alpha_digit_underscore;
	};

	using CommaFile = File<ParseOptions<windows1252_t> { ',', false, '$' }>;
	using ColonFile = File<ParseOptions<windows1252_t> { ':', false, '$' }>;
	using SemiColonFile = File<ParseOptions<windows1252_t> { ';', false, '$' }>;
	using TabFile = File<ParseOptions<windows1252_t> { '\t', false, '$' }>;
	using BarFile = File<ParseOptions<windows1252_t> { '|', false, '$' }>;

	namespace strings {
		using CommaFile = File<ParseOptions<windows1252_t> { ',', true, '$' }>;
		using ColonFile = File<ParseOptions<windows1252_t> { ':', true, '$' }>;
		using SemiColonFile = File<ParseOptions<windows1252_t> { ';', true, '$' }>;
		using TabFile = File<ParseOptions<windows1252_t> { '\t', true, '$' }>;
		using BarFile = File<ParseOptions<windows1252_t> { '|', true, '$' }>;
	}
}

namespace ovdl::csv::grammar::utf8 {
	struct unicode_t {
		static constexpr auto character = lexy::dsl::unicode::character;
		static constexpr auto control = lexy::dsl::unicode::control;
		static constexpr auto xid_start_underscore = lexy::dsl::unicode::xid_start_underscore;
		static constexpr auto xid_continue = lexy::dsl::unicode::xid_continue;
	};

	using CommaFile = File<ParseOptions<unicode_t> { ',', false, '$' }>;
	using ColonFile = File<ParseOptions<unicode_t> { ':', false, '$' }>;
	using SemiColonFile = File<ParseOptions<unicode_t> { ';', false, '$' }>;
	using TabFile = File<ParseOptions<unicode_t> { '\t', false, '$' }>;
	using BarFile = File<ParseOptions<unicode_t> { '|', false, '$' }>;

	namespace strings {
		using CommaFile = File<ParseOptions<unicode_t> { ',', true, '$' }>;
		using ColonFile = File<ParseOptions<unicode_t> { ':', true, '$' }>;
		using SemiColonFile = File<ParseOptions<unicode_t> { ';', true, '$' }>;
		using TabFile = File<ParseOptions<unicode_t> { '\t', true, '$' }>;
		using BarFile = File<ParseOptions<unicode_t> { '|', true, '$' }>;
	}
}