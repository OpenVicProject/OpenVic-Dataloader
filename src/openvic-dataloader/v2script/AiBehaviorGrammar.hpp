#pragma once

#include "ModifierGrammar.hpp"
#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"
#include <lexy/dsl.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace ovdl::v2script::grammar {
	struct AiBehaviorList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<FactorStatement> | lexy::dsl::p<ModifierStatement>);

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::BehaviorListNode>(LEXY_MOV(list));
				});
	};

	struct AiBehaviorBlock {
		static constexpr auto rule = lexy::dsl::curly_bracketed.opt(lexy::dsl::p<AiBehaviorList>);

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto&& list) {
				return LEXY_MOV(list);
			},
			[](lexy::nullopt = {}) {
				return nullptr;
			});
	};
}