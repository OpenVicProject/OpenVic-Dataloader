#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback/forward.hpp>
#include <lexy/dsl.hpp>

#include "ModifierGrammar.hpp"

namespace ovdl::v2script::grammar {
	struct AiBehaviorList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<FactorStatement> | lexy::dsl::p<ModifierStatement>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList>;
	};

	struct AiBehaviorBlock {
		static constexpr auto rule = dsl::curly_bracketed.opt(lexy::dsl::p<AiBehaviorList>);

		static constexpr auto value = construct_list<ast::ListValue>;
	};
}