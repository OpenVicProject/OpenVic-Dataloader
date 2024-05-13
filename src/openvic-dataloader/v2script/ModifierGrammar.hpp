#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback/container.hpp>
#include <lexy/dsl.hpp>

#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include "openvic-dataloader/NodeLocation.hpp"

#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"
#include "detail/dsl.hpp"

namespace ovdl::v2script::grammar {
	constexpr auto modifier_keyword = LEXY_KEYWORD("modifier", id);
	constexpr auto factor_keyword = LEXY_KEYWORD("factor", id);

	struct FactorStatement {
		static constexpr auto rule = lexy::dsl::position(factor_keyword) >> (lexy::dsl::equal_sign + lexy::dsl::p<Identifier<StringEscapeOption>>);
		static constexpr auto value = dsl::callback<ast::AssignStatement*>(
			[](ast::ParseState& state, NodeLocation loc, ast::IdentifierValue* value) {
				auto* factor = state.ast().create<ast::IdentifierValue>(loc, state.ast().intern("factor"));
				return state.ast().create<ast::AssignStatement>(loc, factor, value);
			});
	};

	struct ModifierList {
		struct expected_factor {
			static constexpr auto name = "expected factor in modifier";
		};

		static constexpr auto rule = [] {
			auto factor_flag = lexy::dsl::context_flag<ModifierList>;

			auto element = (lexy::dsl::p<FactorStatement> >> factor_flag.set()) | lexy::dsl::p<TriggerStatement>;

			return dsl::curly_bracketed.list(factor_flag.create() + element) >> lexy::dsl::must(factor_flag.is_reset()).error<expected_factor>;
		}();

		static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue>;
	};

	struct ModifierStatement {
		static constexpr auto rule =
			lexy::dsl::position(modifier_keyword) >> lexy::dsl::equal_sign >> lexy::dsl::p<ModifierList>;

		static constexpr auto value = dsl::callback<ast::AssignStatement*>(
			[](ast::ParseState& state, NodeLocation loc, ast::ListValue* list) {
				auto* factor = state.ast().create<ast::IdentifierValue>(loc, state.ast().intern("modifier"));
				return state.ast().create<ast::AssignStatement>(loc, factor, list);
			});
	};
}