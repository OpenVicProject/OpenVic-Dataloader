#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/callback/adapter.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/callback/container.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/brackets.hpp>
#include <lexy/dsl/option.hpp>
#include <lexy/dsl/position.hpp>
#include <lexy/dsl/production.hpp>

#include "v2script/AiBehaviorGrammar.hpp"
#include "v2script/TriggerGrammar.hpp"

// Decision Grammar Definitions //
namespace ovdl::v2script::grammar {
	struct DecisionStatement {
		using potential = fkeyword_rule<"potential", lexy::dsl::p<TriggerBlock>>;
		using allow = fkeyword_rule<"allow", lexy::dsl::p<TriggerBlock>>;
		using effect = fkeyword_rule<"effect", lexy::dsl::p<TriggerBlock>>;
		using ai_will_do = fkeyword_rule<"ai_will_do", lexy::dsl::p<AiBehaviorBlock>>;

		using helper = dsl::rule_helper<potential, allow, effect, ai_will_do>;

		struct List {
			static constexpr auto rule = dsl::curly_bracketed.opt_list(helper::p | lexy::dsl::p<SimpleAssignmentStatement>);

			static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue>;
		};

		static constexpr auto rule =
			dsl::p<Identifier> >>
			(helper::flags + lexy::dsl::equal_sign + lexy::dsl::p<List>);

		static constexpr auto value = construct<ast::AssignStatement>;
	};

	struct DecisionList {
		static constexpr auto rule =
			ovdl::dsl::keyword<"political_decisions">(id) >>
			(lexy::dsl::equal_sign >> lexy::dsl::curly_bracketed.opt_list(lexy::dsl::p<DecisionStatement>));

		static constexpr auto value = lexy::as_list<ast::AssignStatementList>;
	};

	struct DecisionFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule =
			lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<DecisionList>);

		static constexpr auto value = lexy::concat<ast::AssignStatementList> >> construct<ast::FileTree>;
	};
}