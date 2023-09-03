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
namespace ovdl::csv::grammar::windows1252 {
	constexpr auto character = detail::lexydsl::make_range<0x01, 0xFF>();
	constexpr auto control =
		lexy::dsl::ascii::control /
		lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;

	struct StringValue {
		static constexpr auto escaped_symbols = lexy::symbol_table<char> //
													.map<'"'>('"')
													.map<'\''>('\'')
													.map<'\\'>('\\')
													.map<'/'>('/')
													.map<'b'>('\b')
													.map<'f'>('\f')
													.map<'n'>('\n')
													.map<'r'>('\r')
													.map<'t'>('\t');
		/// This doesn't actually do anything, so this might to be manually parsed if vic2's CSV parser creates a " from ""
		// static constexpr auto escaped_quote = lexy::symbol_table<char> //
		// 										  .map<'"'>('"');
		static constexpr auto rule = [] {
			// Arbitrary code points
			auto c = character - (control / lexy::dsl::lit_b<'"'>);

			auto back_escape = lexy::dsl::backslash_escape //
								   .symbol<escaped_symbols>();

			// auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
			// 						.symbol<escaped_quote>();

			return lexy::dsl::quoted(c, back_escape);
		}();

		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<char Sep>
	struct PlainValue {
		static constexpr auto rule = lexy::dsl::identifier(character - (lexy::dsl::lit_b<Sep> / lexy::dsl::ascii::newline));
		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<char Sep>
	struct Value {
		static constexpr auto rule = lexy::dsl::p<StringValue> | lexy::dsl::p<PlainValue<Sep>>;
		static constexpr auto value = lexy::forward<std::string>;
	};

	template<char Sep>
	struct SepConst {
		static constexpr auto rule = lexy::dsl::lit_b<Sep>;
		static constexpr auto value = lexy::constant(1);
	};

	template<char Sep>
	struct Seperator {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SepConst<Sep>>);
		static constexpr auto value = lexy::count;
	};

	template<char Sep>
	struct LineEnd {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<Value<Sep>>, lexy::dsl::trailing_sep(lexy::dsl::p<Seperator<Sep>>));
		static constexpr auto value = lexy::fold_inplace<csv::LineObject>(
			std::initializer_list<csv::LineObject::value_type> {},
			[](csv::LineObject& result, auto&& arg) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::size_t>) {
					// Count seperators, adds to previous value, making it a position
					using position_type = csv::LineObject::position_type;
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

	template<char Sep>
	struct Line {

		static constexpr auto suffix_setter(csv::LineObject& line) {
			auto& [position, value] = line.back();
			if (value.empty()) {
				line.set_suffix_end(position);
				line.pop_back();
			} else {
				line.set_suffix_end(position + 1);
			}
		};

		static constexpr auto rule = lexy::dsl::p<LineEnd<Sep>> | lexy::dsl::p<Seperator<Sep>> >> lexy::dsl::p<LineEnd<Sep>>;
		static constexpr auto value =
			lexy::callback<csv::LineObject>(
				[](csv::LineObject&& line) {
					suffix_setter(line);
					return LEXY_MOV(line);
				},
				[](std::size_t prefix_count, csv::LineObject&& line) {
					line.set_prefix_end(prefix_count);
					// position needs to be adjusted to prefix
					for (auto& [position, value] : line) {
						position += prefix_count;
					}
					suffix_setter(line);
					return LEXY_MOV(line);
				});
	};

	template<char Sep>
	struct File {
		static constexpr auto rule =
			lexy::dsl::whitespace(lexy::dsl::newline) +
			lexy::dsl::opt(lexy::dsl::list(lexy::dsl::p<Line<Sep>>, lexy::dsl::trailing_sep(lexy::dsl::eol)));

		static constexpr auto value = lexy::as_list<std::vector<csv::LineObject>>;
	};

	using CommaFile = File<','>;
	using ColonFile = File<':'>;
	using SemiColonFile = File<';'>;
	using TabFile = File<'\t'>;
	using BarFile = File<'|'>;
}

namespace ovdl::csv::grammar::utf8 {
	constexpr auto character = lexy::dsl::unicode::character;
	constexpr auto control = lexy::dsl::unicode::control;

	struct StringValue {
		static constexpr auto escaped_symbols = lexy::symbol_table<char> //
													.map<'"'>('"')
													.map<'\''>('\'')
													.map<'\\'>('\\')
													.map<'/'>('/')
													.map<'b'>('\b')
													.map<'f'>('\f')
													.map<'n'>('\n')
													.map<'r'>('\r')
													.map<'t'>('\t');
		/// This doesn't actually do anything, so this might to be manually parsed if vic2's CSV parser creates a " from ""
		// static constexpr auto escaped_quote = lexy::symbol_table<char> //
		// 										  .map<'"'>('"');
		static constexpr auto rule = [] {
			// Arbitrary code points
			auto c = character - (control / lexy::dsl::lit_b<'"'>);

			auto back_escape = lexy::dsl::backslash_escape //
								   .symbol<escaped_symbols>();

			// auto quote_escape = lexy::dsl::escape(lexy::dsl::lit_c<'"'>) //
			// 						.symbol<escaped_quote>();

			return lexy::dsl::quoted(c, back_escape);
		}();

		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<char Sep>
	struct PlainValue {
		static constexpr auto rule = lexy::dsl::identifier(character - (lexy::dsl::lit_b<Sep> / lexy::dsl::ascii::newline));
		static constexpr auto value = lexy::as_string<std::string>;
	};

	template<char Sep>
	struct Value {
		static constexpr auto rule = lexy::dsl::p<StringValue> | lexy::dsl::p<PlainValue<Sep>>;
		static constexpr auto value = lexy::forward<std::string>;
	};

	template<char Sep>
	struct SepConst {
		static constexpr auto rule = lexy::dsl::lit_b<Sep>;
		static constexpr auto value = lexy::constant(1);
	};

	template<char Sep>
	struct Seperator {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SepConst<Sep>>);
		static constexpr auto value = lexy::count;
	};

	template<char Sep>
	struct LineEnd {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<Value<Sep>>, lexy::dsl::trailing_sep(lexy::dsl::p<Seperator<Sep>>));
		static constexpr auto value = lexy::fold_inplace<csv::LineObject>(
			std::initializer_list<csv::LineObject::value_type> {},
			[](csv::LineObject& result, auto&& arg) {
				if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, std::size_t>) {
					// Count seperators, adds to previous value, making it a position
					using position_type = csv::LineObject::position_type;
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

	template<char Sep>
	struct Line {

		static constexpr auto suffix_setter(csv::LineObject& line) {
			auto& [position, value] = line.back();
			if (value.empty()) {
				line.set_suffix_end(position);
				line.pop_back();
			} else {
				line.set_suffix_end(position + 1);
			}
		};

		static constexpr auto rule = lexy::dsl::p<LineEnd<Sep>> | lexy::dsl::p<Seperator<Sep>> >> lexy::dsl::p<LineEnd<Sep>>;
		static constexpr auto value =
			lexy::callback<csv::LineObject>(
				[](csv::LineObject&& line) {
					suffix_setter(line);
					return LEXY_MOV(line);
				},
				[](std::size_t prefix_count, csv::LineObject&& line) {
					line.set_prefix_end(prefix_count);
					// position needs to be adjusted to prefix
					for (auto& [position, value] : line) {
						position += prefix_count;
					}
					suffix_setter(line);
					return LEXY_MOV(line);
				});
	};

	template<char Sep>
	struct File {
		static constexpr auto rule =
			lexy::dsl::whitespace(lexy::dsl::newline) +
			lexy::dsl::opt(lexy::dsl::list(lexy::dsl::p<Line<Sep>>, lexy::dsl::trailing_sep(lexy::dsl::eol)));

		static constexpr auto value = lexy::as_list<std::vector<csv::LineObject>>;
	};

	using CommaFile = File<','>;
	using ColonFile = File<':'>;
	using SemiColonFile = File<';'>;
	using TabFile = File<'\t'>;
	using BarFile = File<'|'>;
}