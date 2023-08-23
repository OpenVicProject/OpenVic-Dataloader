#pragma once

#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <openvic-dataloader/detail/SelfType.hpp>
#include <openvic-dataloader/detail/TypeName.hpp>

#define OVDL_PRINT_FUNC_DEF std::ostream& print(std::ostream& stream, size_t indent) const override

// defines get_type_static and get_type for string type naming
#define OVDL_RT_TYPE_DEF \
	static constexpr std::string_view get_type_static() { return ::ovdl::detail::type_name<type>(); } \
	constexpr std::string_view get_type() const override { return ::ovdl::detail::type_name<std::decay_t<decltype(*this)>>(); }

// defines type for self-class referencing
#define OVDL_TYPE_DEFINE_SELF \
	struct _self_type_tag {}; \
	constexpr auto _self_type_helper()->decltype(::ovdl::detail::Writer<_self_type_tag, decltype(this)> {}); \
	using type = ::ovdl::detail::Read<_self_type_tag>;

namespace ovdl::v2script::ast {

	struct Node;
	using NodePtr = Node*;
	using NodeCPtr = const Node*;
	using NodeUPtr = std::unique_ptr<Node>;

	struct Node {
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		Node() = default;
		Node(Node&&) = default;
		Node& operator=(Node&&) = default;
		virtual ~Node() = default;

		virtual std::ostream& print(std::ostream& stream, size_t indent) const = 0;
		static std::ostream& print_ptr(std::ostream& stream, NodeCPtr node, size_t indent);
		explicit operator std::string() const;

		static constexpr std::string_view get_type_static() { return detail::type_name<Node>(); }
		constexpr virtual std::string_view get_type() const = 0;

		template<typename T>
		constexpr bool is_type() const {
			return get_type().compare(detail::type_name<T>()) == 0;
		}
	};

	inline std::ostream& operator<<(std::ostream& stream, Node const& node) {
		return node.print(stream, 0);
	}
	inline std::ostream& operator<<(std::ostream& stream, NodeCPtr node) {
		return Node::print_ptr(stream, node, 0);
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

	struct IdentifierNode final : public Node {
		std::string _name;
		explicit IdentifierNode(std::string name);

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct StringNode final : public Node {
		std::string _name;
		explicit StringNode(std::string name);

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct AssignNode final : public Node {
		std::string _name;
		NodeUPtr _initializer;
		explicit AssignNode(NodeCPtr name, NodePtr init);

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct ListNode final : public Node {
		std::vector<NodeUPtr> _statements;
		explicit ListNode(std::vector<NodePtr> statements = std::vector<NodePtr> {});

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};

	struct FileNode final : public Node {
		std::vector<NodeUPtr> _statements;
		FileNode() = default;
		explicit FileNode(std::vector<NodePtr> statements);

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;
		OVDL_PRINT_FUNC_DEF;
	};
}

#undef OVDL_PRINT_FUNC_DECL
#undef OVDL_PRINT_FUNC_DEF
#undef OVDL_TYPE_DEFINE_SELF