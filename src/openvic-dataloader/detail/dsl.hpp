#pragma once

#include <concepts> // IWYU pragma: keep
#include <type_traits>

#include <openvic-dataloader/NodeLocation.hpp>

#include <lexy/_detail/config.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/bind.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/callback/fold.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/literal.hpp>
#include <lexy/encoding.hpp>

#include "detail/InternalConcepts.hpp"
#include "detail/StringLiteral.hpp"

namespace ovdl::dsl {
	template<typename ReturnType, typename... Callback>
	constexpr auto callback(Callback... cb) {
		return lexy::bind(lexy::callback<ReturnType>(cb...), lexy::parse_state, lexy::values);
	}

	template<typename Sink>
	constexpr auto bind_sink(Sink sink) {
		return lexy::bind_sink(sink, lexy::parse_state);
	}

	template<typename ReturnT, typename Sink>
	struct _sink_with_state {
		using return_type = ReturnT;

		LEXY_EMPTY_MEMBER Sink _sink_cb;

		template<detail::IsStateType StateType, typename SinkCallback>
		struct _sink_callback {
			StateType& _state;
			SinkCallback _sink_cb;

			using return_type = decltype(LEXY_MOV(_sink_cb).finish());

			template<typename... Args>
			constexpr void operator()(Args&&... args) {
				lexy::_detail::invoke(_sink_cb, _state, LEXY_FWD(args)...);
			}

			constexpr return_type finish() && { return LEXY_MOV(_sink_cb).finish(); }
		};

		template<typename... Args>
		constexpr auto operator()(detail::IsStateType auto& state, Args... args) const -> decltype(_sink_cb(state, LEXY_FWD(args)...)) {
			return _sink_cb(state, LEXY_FWD(args)...);
		}

		constexpr auto sink(detail::IsStateType auto& state) const {
			return _sink_callback<std::decay_t<decltype(state)>, decltype(_sink_cb.sink())> { state, _sink_cb.sink() };
		}
	};

	template<typename ReturnT, typename Sink>
	constexpr auto sink(Sink&& sink) {
		return bind_sink(_sink_with_state<ReturnT, Sink> { LEXY_FWD(sink) });
	}

	template<typename Container, typename Callback>
	constexpr auto collect(Callback callback) {
		return sink(lexy::collect<Container>(callback));
	}

	template<typename Callback>
	constexpr auto collect(Callback callback) {
		return sink(lexy::collect(callback));
	}

	template<typename T>
	constexpr auto construct = callback<T*>(
		[](detail::IsParseState auto& state, ovdl::NodeLocation loc, auto&& arg) {
			if constexpr (std::same_as<std::decay_t<decltype(arg)>, lexy::nullopt>) {
				return state.ast().template create<T>(loc);
			} else {
				return state.ast().template create<T>(loc, DRYAD_FWD(arg));
			}
		},
		[](detail::IsParseState auto& state, ovdl::NodeLocation loc, auto&&... args) {
			return state.ast().template create<T>(loc, DRYAD_FWD(args)...);
		});

	template<typename T, typename ListType, bool DisableEmpty = false>
	constexpr auto construct_list = callback<T*>(
		[](detail::IsParseState auto& state, const char* begin, ListType&& arg, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end), DRYAD_FWD(arg));
		},
		[](detail::IsParseState auto& state, const char* begin, lexy::nullopt, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		},
		[](detail::IsParseState auto& state, const char* begin, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		},
		[](detail::IsParseState auto& state) {
			return nullptr;
		});

	template<typename T, typename ListType>
	constexpr auto construct_list<T, ListType, true> = callback<T*>(
		[](detail::IsParseState auto& state, const char* begin, ListType&& arg, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end), DRYAD_FWD(arg));
		},
		[](detail::IsParseState auto& state, const char* begin, lexy::nullopt, const char* end) {
			return state.ast().template create<T>(NodeLocation::make_from(begin, end));
		});

	template<typename CharT, CharT LowC, CharT HighC>
	struct _crange : lexyd::char_class_base<_crange<CharT, LowC, HighC>> {
		static_assert(LowC >= 0, "LowC cannot be less than 0");
		static_assert(HighC - LowC > 0, "LowC must be less than HighC");

		static constexpr auto char_class_unicode() {
			return LowC <= 0x7F && HighC <= 0x7F;
		}

		static LEXY_CONSTEVAL auto char_class_name() {
			return "range";
		}

		static LEXY_CONSTEVAL auto char_class_ascii() {
			lexy::_detail::ascii_set result;
			if constexpr (LowC <= 0x7F && HighC <= 0x7F) {
				for (auto c = LowC; c <= HighC; c++) {
					result.insert(c);
				}
			}
			return result;
		}

		static constexpr auto char_class_match_cp([[maybe_unused]] char32_t cp) {
			if constexpr (LowC <= 0x7F && HighC <= 0x7F) {
				return std::false_type {};
			} else {
				return LowC <= cp && cp <= HighC;
			}
		}
	};

	template<auto LowC, decltype(LowC) HighC>
	constexpr auto lit_c_range = _crange<LEXY_DECAY_DECLTYPE(LowC), LowC, HighC> {};

	template<unsigned char LowC, unsigned char HighC>
	constexpr auto lit_b_range = _crange<unsigned char, LowC, HighC> {};

	template<auto Open, auto Close>
	constexpr auto position_brackets = lexy::dsl::brackets(lexy::dsl::position(lexy::dsl::lit_c<Open>), lexy::dsl::position(lexy::dsl::lit_c<Close>));

	constexpr auto round_bracketed = position_brackets<'(', ')'>;
	constexpr auto square_bracketed = position_brackets<'[', ']'>;
	constexpr auto curly_bracketed = position_brackets<'{', '}'>;
	constexpr auto angle_bracketed = position_brackets<'<', '>'>;

	template<typename Production>
	constexpr auto p = lexy::dsl::position(lexy::dsl::p<Production>);

	template<typename ReturnType, ovdl::detail::string_literal Keyword>
	static constexpr auto default_kw_value = dsl::callback<ReturnType*>(
		[](detail::IsParseState auto& state, NodeLocation loc) {
			return state.ast().template create<ReturnType>(loc, state.ast().intern(Keyword.data(), Keyword.size()));
		});

	template<
		auto Identifier,
		typename RuleValue,
		ovdl::detail::string_literal Keyword,
		auto Production,
		auto Value>
	struct keyword_rule {
		struct rule_t {
			static constexpr auto keyword = ovdl::dsl::keyword<Keyword>(Identifier);
			static constexpr auto rule = lexy::dsl::position(keyword) >> lexy::dsl::equal_sign;
			static constexpr auto value = Value;
		};
		static constexpr auto rule = dsl::p<rule_t> >> Production;
		static constexpr auto value = construct<RuleValue>;
	};

	template<
		auto Identifier,
		typename RuleValue,
		ovdl::detail::string_literal Keyword,
		auto Production,
		auto Value>
	struct fkeyword_rule : keyword_rule<Identifier, RuleValue, Keyword, Production, Value> {
		using base_type = keyword_rule<Identifier, RuleValue, Keyword, Production, Value>;
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
		static constexpr auto value = construct<RuleValue>;
	};

	template<typename... Args>
	struct rule_helper {
		static constexpr auto flags = (Args::make_flag + ...);
		static constexpr auto p = (lexy::dsl::p<Args> | ...);
	};

	template<typename Rule, typename RuleUtf, typename Tag>
	struct _peek : lexyd::branch_base {
		template<typename Reader>
		struct bp {
			typename Reader::iterator begin;
			typename Reader::marker end;

			constexpr bool try_parse(const void*, Reader reader) {
				using encoding = typename Reader::encoding;

				auto parser = [&] {
					if constexpr (std::same_as<encoding, lexy::default_encoding> || std::same_as<encoding, lexy::byte_encoding>) {
						// We need to match the entire rule.
						return lexy::token_parser_for<decltype(lexy::dsl::token(Rule {})), Reader> { reader };
					} else {
						// We need to match the entire rule.
						return lexy::token_parser_for<decltype(lexy::dsl::token(RuleUtf {})), Reader> { reader };
					}
				}();

				begin = reader.position();
				auto result = parser.try_parse(reader);
				end = parser.end;

				return result;
			}

			template<typename Context>
			constexpr void cancel(Context& context) {
				context.on(lexyd::_ev::backtracked {}, begin, end.position());
			}

			template<typename NextParser, typename Context, typename... Args>
			LEXY_PARSER_FUNC bool finish(Context& context, Reader& reader, Args&&... args) {
				context.on(lexyd::_ev::backtracked {}, begin, end.position());
				return NextParser::parse(context, reader, LEXY_FWD(args)...);
			}
		};

		template<typename NextParser>
		struct p {
			template<typename Context, typename Reader, typename... Args>
			LEXY_PARSER_FUNC static bool parse(Context& context, Reader& reader, Args&&... args) {
				bp<Reader> impl {};
				if (!impl.try_parse(context.control_block, reader)) {
					// Report that we've failed.
					using tag = lexy::_detail::type_or<Tag, lexy::peek_failure>;
					auto err = lexy::error<Reader, tag>(impl.begin, impl.end.position());
					context.on(lexyd::_ev::error {}, err);

					// But recover immediately, as we wouldn't have consumed anything either way.
				}

				context.on(lexyd::_ev::backtracked {}, impl.begin, impl.end);
				return NextParser::parse(context, reader, LEXY_FWD(args)...);
			}
		};

		template<typename Error>
		static constexpr _peek<Rule, RuleUtf, Error> error = {};
	};

	template<typename Rule, typename RuleUtf>
	constexpr auto peek(Rule, RuleUtf) {
		return _peek<Rule, RuleUtf, void> {};
	}
}