#pragma once

#include <type_traits>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/ParseState.hpp>

#include <lexy/callback/adapter.hpp>
#include <lexy/callback/bind.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/fold.hpp>
#include <lexy/dsl.hpp>

#include "detail/StringLiteral.hpp"

namespace ovdl::dsl {
	template<typename ReturnType, typename... Callback>
	constexpr auto callback(Callback... cb) {
		return lexy::bind(lexy::callback<ReturnType>(cb...), lexy::parse_state, lexy::values);
	}

	template<typename Sink>
	constexpr auto sink(Sink sink) {
		return lexy::bind_sink(sink, lexy::parse_state);
	}

	template<typename Container, typename Callback>
	constexpr auto collect(Callback callback) {
		return sink(lexy::collect<Container>(callback));
	}

	template<typename Callback>
	constexpr auto collect(Callback callback) {
		return sink(lexy::collect(callback));
	}

	template<IsParseState StateType, typename T>
	constexpr auto construct = callback<T*>(
		[](StateType& state, ovdl::NodeLocation loc, auto&& arg) {
			if constexpr (std::is_same_v<std::decay_t<decltype(arg)>, lexy::nullopt>)
				return state.ast().template create<T>(loc);
			else
				return state.ast().template create<T>(loc, DRYAD_FWD(arg));
		},
		[](StateType& state, ovdl::NodeLocation loc, auto&&... args) {
			return state.ast().template create<T>(loc, DRYAD_FWD(args)...);
		});

	template<IsParseState StateType, typename T, typename ListType, bool DisableEmpty = false>
	constexpr auto construct_list = callback<T*>(
		[](StateType& state, const char* begin, ListType&& arg, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end), DRYAD_FWD(arg));
		},
		[](StateType& state, const char* begin, lexy::nullopt, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		},
		[](StateType& state, const char* begin, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		});

	template<IsParseState StateType, typename T, typename ListType>
	constexpr auto construct_list<StateType, T, ListType, true> = callback<T*>(
		[](StateType& state, const char* begin, ListType&& arg, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end), DRYAD_FWD(arg));
		},
		[](StateType& state, const char* begin, lexy::nullopt, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		});

	template<unsigned char LOW, unsigned char HIGH>
	consteval auto make_range() {
		if constexpr (LOW == HIGH) {
			return ::lexy::dsl::lit_c<LOW>;
		} else if constexpr (LOW == (HIGH - 1)) {
			return ::lexy::dsl::lit_c<LOW> / ::lexy::dsl::lit_c<HIGH>;
		} else {
			return ::lexy::dsl::lit_c<LOW> / make_range<LOW + 1, HIGH>();
		}
	}

	template<auto Open, auto Close>
	constexpr auto position_brackets = lexy::dsl::brackets(lexy::dsl::position(lexy::dsl::lit_c<Open>), lexy::dsl::position(lexy::dsl::lit_c<Close>));

	constexpr auto round_bracketed = position_brackets<'(', ')'>;
	constexpr auto square_bracketed = position_brackets<'[', ']'>;
	constexpr auto curly_bracketed = position_brackets<'{', '}'>;
	constexpr auto angle_bracketed = position_brackets<'<', '>'>;

	template<typename Production>
	constexpr auto p = lexy::dsl::position(lexy::dsl::p<Production>);

	template<IsParseState ParseType, typename ReturnType, ovdl::detail::string_literal Keyword>
	static constexpr auto default_kw_value = dsl::callback<ReturnType*>(
		[](ParseType& state, NodeLocation loc) {
			return state.ast().template create<ReturnType>(loc, state.ast().intern(Keyword.data(), Keyword.size()));
		});

	template<
		IsParseState ParseType,
		typename Identifier,
		typename RuleValue,
		ovdl::detail::string_literal Keyword,
		auto Production,
		auto Value>
	struct keyword_rule {
		struct rule_t {
			static constexpr auto keyword = ovdl::dsl::keyword<Keyword>(lexy::dsl::inline_<Identifier>);
			static constexpr auto rule = lexy::dsl::position(keyword) >> lexy::dsl::equal_sign;
			static constexpr auto value = Value;
		};
		static constexpr auto rule = dsl::p<rule_t> >> Production;
		static constexpr auto value = construct<ParseType, RuleValue>;
	};

	template<
		IsParseState ParseType,
		typename Identifier,
		typename RuleValue,
		ovdl::detail::string_literal Keyword,
		auto Production,
		auto Value>
	struct fkeyword_rule : keyword_rule<ParseType, Identifier, RuleValue, Keyword, Production, Value> {
		using base_type = keyword_rule<ParseType, Identifier, RuleValue, Keyword, Production, Value>;
		struct context_t;
		struct rule_t : base_type::rule_t {
			static constexpr auto flag = lexy::dsl::context_flag<context_t>;
			struct too_many_error {
				static constexpr auto name = "expected event " + Keyword + " to only be found once";
			};
			static constexpr auto must = lexy::dsl::must(rule_t::flag.is_reset())
											 .
// See https://stackoverflow.com/questions/77144003/use-of-template-keyword-before-dependent-template-name
// THANKS FOR NOTHING MICROSOFT, CAN'T EVEN GET THE STANDARD RIGHT
#if !defined(_MSC_VER) || defined(__clang__)
										 template
#endif
										 error<too_many_error>;
		};
		static constexpr auto make_flag = rule_t::flag.create();

		static constexpr auto rule = dsl::p<rule_t> >> (rule_t::must >> rule_t::flag.set()) >> Production;
		static constexpr auto value = construct<ParseType, RuleValue>;
	};

	template<typename... Args>
	struct rule_helper {
		static constexpr auto flags = (Args::make_flag + ...);
		static constexpr auto p = (lexy::dsl::p<Args> | ...);
	};
}