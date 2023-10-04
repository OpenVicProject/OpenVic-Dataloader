#pragma once

#include <cstddef>
#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <openvic-dataloader/detail/OptionalConstexpr.hpp>
#include <openvic-dataloader/detail/SelfType.hpp>
#include <openvic-dataloader/detail/TypeName.hpp>

namespace lexy {
	struct nullopt;
}

namespace ovdl::v2script {
	class Parser;
}

#define OVDL_PRINT_FUNC_DEF std::ostream& print(std::ostream& stream, std::size_t indent) const override

// defines get_type_static and get_type for string type naming
#define OVDL_RT_TYPE_DEF                                                                              \
	static constexpr std::string_view get_type_static() { return ::ovdl::detail::type_name<type>(); } \
	constexpr std::string_view get_type() const override { return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }

// defines type for self-class referencing
#define OVDL_TYPE_DEFINE_SELF                                                                                \
	struct _self_type_tag {};                                                                                \
	constexpr auto _self_type_helper()->decltype(::ovdl::detail::Writer<_self_type_tag, decltype(this)> {}); \
	using type = ::ovdl::detail::Read<_self_type_tag>;

namespace ovdl::v2script::ast {

	struct Node;
	using NodePtr = Node*;
	using NodeCPtr = const Node*;
	using NodeUPtr = std::unique_ptr<Node>;

	struct NodeLocation {
		const char* _begin = nullptr;
		const char* _end = nullptr;

		NodeLocation() = default;
		NodeLocation(const char* pos) : _begin(pos),
										_end(pos) {}
		NodeLocation(const char* begin, const char* end) : _begin(begin),
														   _end(end) {}

		NodeLocation(const NodeLocation&) = default;
		NodeLocation& operator=(const NodeLocation&) = default;

		NodeLocation(NodeLocation&&) = default;
		NodeLocation& operator=(NodeLocation&&) = default;

		const char* begin() const { return _begin; }
		const char* end() const { return _end; }

		static inline NodeLocation make_from(const char* begin, const char* end) {
			end++;
			if (begin >= end) return NodeLocation(begin);
			return NodeLocation(begin, end);
		}
	};

	struct Node {
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		Node(NodeLocation location) : _location(location) {}
		Node(Node&&) = default;
		Node& operator=(Node&&) = default;
		virtual ~Node() = default;

		virtual std::ostream& print(std::ostream& stream, std::size_t indent) const = 0;
		static std::ostream& print_ptr(std::ostream& stream, NodeCPtr node, std::size_t indent);
		explicit operator std::string() const;

		static constexpr std::string_view get_type_static() { return detail::type_name<Node>(); }
		constexpr virtual std::string_view get_type() const = 0;

		static constexpr std::string_view get_base_type_static() { return detail::type_name<Node>(); }
		constexpr virtual std::string_view get_base_type() const { return get_base_type_static(); }

		template<typename T>
		constexpr bool is_type() const {
			return get_type().compare(detail::type_name<T>()) == 0;
		}

		template<typename T>
		constexpr bool is_derived_from() const {
			return is_type<T>() || get_base_type().compare(detail::type_name<T>()) == 0;
		}

		template<typename T>
		constexpr T* cast_to() {
			if (is_derived_from<T>() || is_type<Node>()) return (static_cast<T*>(this));
			return nullptr;
		}

		template<typename T>
		constexpr const T* const cast_to() const {
			if (is_derived_from<T>() || is_type<Node>()) return (static_cast<const T*>(this));
			return nullptr;
		}

		const NodeLocation location() const { return _location; }

		struct line_col {
			uint32_t line;
			uint32_t column;
		};

	private:
		friend class ::ovdl::v2script::Parser;
		const line_col get_begin_line_col(const Parser& parser) const;
		const line_col get_end_line_col(const Parser& parser) const;

	private:
		NodeLocation _location;
	};

	inline std::ostream& operator<<(std::ostream& stream, Node const& node) {
		return node.print(stream, 0);
	}
	inline std::ostream& operator<<(std::ostream& stream, NodeCPtr node) {
		return Node::print_ptr(stream, node, 0);
	}
	inline std::ostream& operator<<(std::ostream& stream, Node::line_col const& val) {
		return stream << '(' << val.line << ':' << val.column << ')';
	}

	template<class T, class... Args>
	NodePtr make_node_ptr(Args&&... args) {
		if constexpr (std::is_pointer_v<NodePtr>) {
			return new T(std::forward<Args>(args)...);
		} else {
			return NodePtr(new T(std::forward<Args>(args)...));
		}
	}

	template<typename To, typename From>
	To& cast_node_ptr(const From& from) {
		if constexpr (std::is_pointer_v<NodePtr>) {
			return *static_cast<To*>(from);
		} else {
			return *static_cast<To*>(from.get());
		}
	}

	template<typename To, typename From>
	const To& cast_node_cptr(const From& from) {
		if constexpr (std::is_pointer_v<NodePtr>) {
			return *static_cast<const To*>(from);
		} else {
			return *static_cast<const To*>(from.get());
		}
	}

	void copy_into_node_ptr_vector(const std::vector<NodePtr>& source, std::vector<NodeUPtr>& dest);

	struct AbstractStringNode : public Node {
		std::string _name;
		AbstractStringNode();
		AbstractStringNode(std::string&& name);
		AbstractStringNode(NodeLocation location);
		AbstractStringNode(NodeLocation location, std::string&& name);
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
		static constexpr std::string_view get_base_type_static() { return detail::type_name<AbstractStringNode>(); }
		constexpr std::string_view get_base_type() const override { return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }
	};

#define OVDL_AST_STRING_NODE(NAME)                       \
	struct NAME final : public AbstractStringNode {      \
		NAME();                                          \
		NAME(std::string&& name);                        \
		NAME(lexy::nullopt);                             \
		NAME(NodeLocation location);                     \
		NAME(NodeLocation location, std::string&& name); \
		NAME(NodeLocation location, lexy::nullopt);      \
		OVDL_TYPE_DEFINE_SELF;                           \
		OVDL_RT_TYPE_DEF;                                \
		OVDL_PRINT_FUNC_DEF;                             \
	}

	// Value Expression Nodes
	OVDL_AST_STRING_NODE(IdentifierNode);
	OVDL_AST_STRING_NODE(StringNode);

	// Assignment Nodes
	OVDL_AST_STRING_NODE(FactorNode);
	OVDL_AST_STRING_NODE(MonthNode);
	OVDL_AST_STRING_NODE(NameNode);
	OVDL_AST_STRING_NODE(FireOnlyNode);
	OVDL_AST_STRING_NODE(IdNode);
	OVDL_AST_STRING_NODE(TitleNode);
	OVDL_AST_STRING_NODE(DescNode);
	OVDL_AST_STRING_NODE(PictureNode);
	OVDL_AST_STRING_NODE(IsTriggeredNode);

#undef OVDL_AST_STRING_NODE

	struct AssignNode final : public Node {
		std::string _name;
		NodeUPtr _initializer;
		AssignNode(NodeLocation location, NodeCPtr name, NodePtr init);
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct AbstractListNode : public Node {
		std::vector<NodeUPtr> _statements;
		AbstractListNode(const std::vector<NodePtr>& statements = std::vector<NodePtr> {});
		AbstractListNode(NodeLocation location, const std::vector<NodePtr>& statements = std::vector<NodePtr> {});
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
		static constexpr std::string_view get_base_type_static() { return detail::type_name<AbstractListNode>(); }
		constexpr std::string_view get_base_type() const override { return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }
	};

#define OVDL_AST_LIST_NODE(NAME)                                                                       \
	struct NAME final : public AbstractListNode {                                                      \
		NAME(const std::vector<NodePtr>& statements = std::vector<NodePtr> {});                        \
		NAME(lexy::nullopt);                                                                           \
		NAME(NodeLocation location, const std::vector<NodePtr>& statements = std::vector<NodePtr> {}); \
		NAME(NodeLocation location, lexy::nullopt);                                                    \
		OVDL_TYPE_DEFINE_SELF;                                                                         \
		OVDL_RT_TYPE_DEF;                                                                              \
		OVDL_PRINT_FUNC_DEF;                                                                           \
	}

	OVDL_AST_LIST_NODE(FileNode);
	OVDL_AST_LIST_NODE(ListNode);

	OVDL_AST_LIST_NODE(ModifierNode);
	OVDL_AST_LIST_NODE(MtthNode);
	OVDL_AST_LIST_NODE(EventOptionNode);
	OVDL_AST_LIST_NODE(BehaviorListNode);
	OVDL_AST_LIST_NODE(DecisionListNode);

#undef OVDL_AST_LIST_NODE

	struct EventNode final : public Node {
		enum class Type {
			Country,
			Province
		} _type;
		std::vector<NodeUPtr> _statements;
		EventNode(Type type, const std::vector<NodePtr>& statements = {});
		EventNode(NodeLocation location, Type type, const std::vector<NodePtr>& statements = {});
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct DecisionNode final : public Node {
		NodeUPtr _name;
		std::vector<NodeUPtr> _statements;
		DecisionNode(NodePtr name, const std::vector<NodePtr>& statements = {});
		DecisionNode(NodeLocation location, NodePtr name, const std::vector<NodePtr>& statements = {});
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct EventMtthModifierNode final : public Node {
		NodeUPtr _factor_value;
		std::vector<NodeUPtr> _statements;
		EventMtthModifierNode() : Node({}) {}
		EventMtthModifierNode(NodeLocation location) : Node(location) {}
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	// Packed single case
	struct ExecutionNode final : public Node {
		enum class Type {
			Effect,
			Trigger
		} _type;
		NodeUPtr _name;
		NodeUPtr _initializer;
		ExecutionNode(Type type, NodePtr name, NodePtr init);
		ExecutionNode(NodeLocation location, Type type, NodePtr name, NodePtr init);
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct ExecutionListNode final : public Node {
		ExecutionNode::Type _type;
		std::vector<NodeUPtr> _statements;
		ExecutionListNode(ExecutionNode::Type type, const std::vector<NodePtr>& statements);
		ExecutionListNode(NodeLocation location, ExecutionNode::Type type, const std::vector<NodePtr>& statements);
		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

}

#undef OVDL_PRINT_FUNC_DECL
#undef OVDL_PRINT_FUNC_DEF
#undef OVDL_TYPE_DEFINE_SELF