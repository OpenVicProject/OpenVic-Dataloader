#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "SimpleGrammar.hpp"

namespace ovdl::v2script::grammar {
	struct EffectStatement {
		static constexpr auto rule = lexy::dsl::inline_<SimpleAssignmentStatement<StringEscapeOption>>;

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto name, auto&& initalizer) {
				return ast::make_node_ptr<ast::ExecutionNode>(ast::ExecutionNode::Type::Effect, LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	struct EffectList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<SimpleAssignmentStatement<StringEscapeOption>>);

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::ExecutionListNode>(ast::ExecutionNode::Type::Effect, LEXY_MOV(list));
				});
	};

	struct EffectBlock {
		static constexpr auto rule = lexy::dsl::curly_bracketed.opt(lexy::dsl::p<EffectList>);

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto&& list) {
				return LEXY_MOV(list);
			},
			[](lexy::nullopt = {}) {
				return lexy::nullopt {};
			});
	};
}