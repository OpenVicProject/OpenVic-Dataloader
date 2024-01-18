#pragma once

#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/option.hpp>

#include "AiBehaviorGrammar.hpp"
#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"

// Decision Grammar Definitions //
namespace ovdl::v2script::grammar {
	//////////////////
	// Macros
	//////////////////
// Produces <KW_NAME>_rule and <KW_NAME>_p
#define OVDL_GRAMMAR_KEYWORD_DEFINE(KW_NAME)                                                                        \
	struct KW_NAME##_rule {                                                                                         \
		static constexpr auto keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier<StringEscapeOption>>); \
		static constexpr auto rule = keyword >> lexy::dsl::equal_sign;                                              \
		static constexpr auto value = lexy::noop;                                                                   \
	};                                                                                                              \
	static constexpr auto KW_NAME##_p = lexy::dsl::p<KW_NAME##_rule>

// Produces <KW_NAME>_rule and <KW_NAME>_p and <KW_NAME>_rule::flag and <KW_NAME>_rule::too_many_error
#define OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(KW_NAME)                                                                   \
	struct KW_NAME##_rule {                                                                                         \
		static constexpr auto keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier<StringEscapeOption>>); \
		static constexpr auto rule = keyword >> lexy::dsl::equal_sign;                                              \
		static constexpr auto value = lexy::noop;                                                                   \
		static constexpr auto flag = lexy::dsl::context_flag<struct KW_NAME##_context>;                             \
		struct too_many_error {                                                                                     \
			static constexpr auto name = "expected left side " #KW_NAME " to be found once";                        \
		};                                                                                                          \
	};                                                                                                              \
	static constexpr auto KW_NAME##_p = lexy::dsl::p<KW_NAME##_rule> >> (lexy::dsl::must(KW_NAME##_rule::flag.is_reset()).error<KW_NAME##_rule::too_many_error> + KW_NAME##_rule::flag.set())
	//////////////////
	// Macros
	//////////////////
	struct DecisionStatement {
		template<auto Production, typename AstNode>
		struct _StringStatement {
			static constexpr auto rule = Production >> (lexy::dsl::p<StringExpression<StringEscapeOption>> | lexy::dsl::p<Identifier<StringEscapeOption>>);
			static constexpr auto value = lexy::forward<ast::NodePtr>;
		};
		template<auto Production, typename AstNode>
		static constexpr auto StringStatement = lexy::dsl::p<_StringStatement<Production, AstNode>>;

		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(potential);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(allow);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(effect);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(ai_will_do);

		static constexpr auto rule = [] {
			constexpr auto create_flags =
				potential_rule::flag.create() +
				allow_rule::flag.create() +
				effect_rule::flag.create() +
				ai_will_do_rule::flag.create();

			constexpr auto potential_statement = potential_p >> lexy::dsl::p<TriggerBlock>;
			constexpr auto allow_statement = allow_p >> lexy::dsl::p<TriggerBlock>;
			constexpr auto effect_statement = effect_p >> lexy::dsl::p<TriggerBlock>;
			constexpr auto ai_will_do_statement = ai_will_do_p >> lexy::dsl::p<AiBehaviorBlock>;

			return lexy::dsl::p<Identifier<StringEscapeOption>> >>
				   (create_flags + lexy::dsl::equal_sign +
					   lexy::dsl::curly_bracketed.list(
						   potential_statement |
						   allow_statement |
						   effect_statement |
						   ai_will_do_statement |
						   lexy::dsl::p<SimpleAssignmentStatement<StringEscapeOption>>));
		}();

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& name, auto&& list) {
					return ast::make_node_ptr<ast::DecisionNode>(LEXY_MOV(name), LEXY_MOV(list));
				},
				[](auto&& name, lexy::nullopt = {}) {
					return ast::make_node_ptr<ast::DecisionNode>(LEXY_MOV(name));
				});
	};

	struct DecisionList {
		static constexpr auto rule =
			LEXY_KEYWORD("political_decisions", lexy::dsl::inline_<Identifier<StringEscapeOption>>) >>
			(lexy::dsl::equal_sign + lexy::dsl::curly_bracketed.opt_list(lexy::dsl::p<DecisionStatement>));

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::DecisionListNode>(LEXY_MOV(list));
				},
				[](lexy::nullopt = {}) {
					return lexy::nullopt {};
				});
	};

	struct DecisionFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule =
			lexy::dsl::terminator(lexy::dsl::eof).list( //
				lexy::dsl::p<DecisionList> |			//
				lexy::dsl::p<SimpleAssignmentStatement<StringEscapeOption>>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};

#undef OVDL_GRAMMAR_KEYWORD_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_STATEMENT
#undef OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT
}