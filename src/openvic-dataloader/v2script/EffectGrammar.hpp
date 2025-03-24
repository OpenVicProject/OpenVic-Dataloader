#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "SimpleGrammar.hpp"
#include "detail/dsl.hpp"

namespace ovdl::v2script::grammar {
	struct EffectStatement {
		static constexpr auto rule = lexy::dsl::p<SimpleAssignmentStatement>;

		static constexpr auto value = lexy::forward<ast::AssignStatement*>;
	};

	struct EffectList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<EffectStatement>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList>;
	};

	struct EffectBlock {
		static constexpr auto rule = dsl::curly_bracketed.opt(lexy::dsl::p<EffectList>);

		static constexpr auto value = construct_list<ast::ListValue>;
	};
}