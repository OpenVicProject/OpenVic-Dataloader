#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "SimpleGrammar.hpp"
#include "detail/dsl.hpp"

namespace ovdl::v2script::grammar {
	struct TriggerStatement {
		static constexpr auto rule = lexy::dsl::p<SAssignStatement<StringEscapeOption>>;

		static constexpr auto value = lexy::forward<ast::AssignStatement*>;
	};

	struct TriggerList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<TriggerStatement>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList>;
	};

	struct TriggerBlock {
		static constexpr auto rule = dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>);

		static constexpr auto value = construct_list<ast::ListValue>;
	};
}