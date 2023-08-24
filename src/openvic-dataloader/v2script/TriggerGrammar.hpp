#pragma once

#include "SimpleGrammar.hpp"
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace ovdl::v2script::grammar {
	struct TriggerStatement {
		static constexpr auto rule = lexy::dsl::inline_<SimpleAssignmentStatement>;

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto name, auto&& initalizer) {
				return ast::make_node_ptr<ast::ExecutionNode>(ast::ExecutionNode::Type::Trigger, LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	struct TriggerList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SimpleAssignmentStatement>);

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::ExecutionListNode>(ast::ExecutionNode::Type::Trigger, LEXY_MOV(list));
				});
	};

	struct TriggerBlock {
		static constexpr auto rule = lexy::dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>);

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto&& list) {
				return LEXY_MOV(list);
			},
			[](lexy::nullopt = {}) {
				return lexy::nullopt {};
			});
	};
}