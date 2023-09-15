#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/dsl.hpp>

#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"

namespace ovdl::v2script::grammar {
	constexpr auto modifier_keyword = LEXY_KEYWORD("modifier", lexy::dsl::inline_<Identifier<StringEscapeOption>>);
	constexpr auto factor_keyword = LEXY_KEYWORD("factor", lexy::dsl::inline_<Identifier<StringEscapeOption>>);

	struct FactorStatement {
		static constexpr auto rule = factor_keyword >> lexy::dsl::equal_sign + lexy::dsl::inline_<Identifier<StringEscapeOption>>;
		static constexpr auto value = lexy::as_string<std::string> | lexy::new_<ast::FactorNode, ast::NodePtr>;
	};

	struct ModifierStatement {
		static constexpr auto rule =
			modifier_keyword >>
			lexy::dsl::curly_bracketed.list(
				lexy::dsl::p<FactorStatement> |
				lexy::dsl::p<TriggerList>);

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::ModifierNode>(LEXY_MOV(list));
				});
	};
}