#pragma once

#include <unordered_map>

#include <openvic-dataloader/detail/PointerHash.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/input_location.hpp>

namespace ovdl::v2script::ast {
	// TODO: FOR THE LOVE OF GOD USE A DIFFERENT HASH MULTIMAP TYPE
	// See src/openvic-dataloader/v2script/Parser.cpp#252
	template<typename Input>
	struct NodeLocationMap : public std::unordered_multimap<NodeCPtr, const lexy::input_location<Input>, detail::PointerHash<Node>> {
		NodeLocationMap() = default;
		NodeLocationMap(const Input& input, const Node& top_node) {
			generate_location_map(input, top_node);
		}

		NodeLocationMap(const NodeLocationMap&) = default;
		NodeLocationMap(NodeLocationMap&&) = default;

		NodeLocationMap& operator=(const NodeLocationMap&) = default;
		NodeLocationMap& operator=(NodeLocationMap&&) = default;

		lexy::input_location_anchor<Input> generate_location_map(const Input& input, NodeCPtr node);
		lexy::input_location_anchor<Input> generate_location_map(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor);
		lexy::input_location_anchor<Input> generate_begin_location_for(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor);
		lexy::input_location_anchor<Input> generate_end_location_for(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor);
	};

	template<typename Input>
	constexpr const lexy::input_location<Input> make_begin_loc(const NodeLocation location, const Input& input, lexy::input_location_anchor<Input> anchor) {
		return lexy::get_input_location(input, location.begin(), anchor);
	}

	template<typename Input>
	constexpr const lexy::input_location<Input> make_begin_loc(const NodeLocation location, const Input& input) {
		return lexy::get_input_location(input, location.begin());
	}

	template<typename Input>
	constexpr const lexy::input_location<Input> make_end_loc(const NodeLocation location, const Input& input, lexy::input_location_anchor<Input> anchor) {
		return lexy::get_input_location(input, location.end(), anchor);
	}

	template<typename Input>
	constexpr const lexy::input_location<Input> make_end_loc(const NodeLocation location, const Input& input) {
		return lexy::get_input_location(input, location.end());
	}
}

namespace ovdl::v2script::ast {
	template<typename Input>
	lexy::input_location_anchor<Input> NodeLocationMap<Input>::generate_location_map(const Input& input, NodeCPtr node) {
		return generate_location_map(input, node, lexy::input_location_anchor(input));
	}

	template<typename Input>
	lexy::input_location_anchor<Input> NodeLocationMap<Input>::generate_location_map(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor) {
		if (!node) return anchor;
		anchor = generate_begin_location_for(input, node, anchor);
		if (auto list_node = node->cast_to<ast::AbstractListNode>(); list_node) {
			for (auto& inner_node : list_node->_statements) {
				anchor = generate_location_map(input, inner_node.get(), anchor);
			}
		} else if (auto assign_node = node->cast_to<ast::AssignNode>(); assign_node) {
			anchor = generate_location_map(input, assign_node->_initializer.get(), anchor);
		}
		// TODO: implement for EventNode, DecisionNode, EventMtthModifierNode, ExecutionNode, ExecutionListNode
		if (!node->location().end() || node->location().begin() >= node->location().end())
			return anchor;
		return generate_end_location_for(input, node, anchor);
	}

	template<typename Input>
	lexy::input_location_anchor<Input> NodeLocationMap<Input>::generate_begin_location_for(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor) {
		if (node->location().begin() == nullptr) return anchor;
		lexy::input_location<Input> next_loc = make_begin_loc(node->location(), input, anchor);
		this->emplace(node, next_loc);
		return next_loc.anchor();
	}

	template<typename Input>
	lexy::input_location_anchor<Input> NodeLocationMap<Input>::generate_end_location_for(const Input& input, NodeCPtr node, lexy::input_location_anchor<Input> anchor) {
		if (node->location().end() == nullptr) return anchor;
		lexy::input_location<Input> next_loc = make_end_loc(node->location(), input, anchor);
		this->emplace(node, next_loc);
		return next_loc.anchor();
	}
}