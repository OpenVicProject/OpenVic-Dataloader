#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

#include <openvic-dataloader/detail/SelfType.hpp>
#include <openvic-dataloader/detail/TypeName.hpp>

#ifdef OPENVIC_DATALOADER_PRINT_NODES
#include <iostream>

#define OVDL_PRINT_FUNC_DECL virtual void print(std::ostream& stream) const = 0
#define OVDL_PRINT_FUNC_DEF(...) \
	void print(std::ostream& stream) const override __VA_ARGS__
#else
#define OVDL_PRINT_FUNC_DECL
#define OVDL_PRINT_FUNC_DEF(...)
#endif

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
	struct Node {
		Node(const Node&) = delete;
		Node& operator=(const Node&) = delete;
		Node() = default;
		Node(Node&&) = default;
		Node& operator=(Node&&) = default;
		virtual ~Node() = default;

		OVDL_PRINT_FUNC_DECL;

		static constexpr std::string_view get_type_static() { return detail::type_name<Node>(); }
		constexpr virtual std::string_view get_type() const = 0;

		template<typename T>
		constexpr bool is_type() const {
			return get_type().compare(detail::type_name<T>()) == 0;
		}
	};

	using NodePtr = Node*;
	using NodeUPtr = std::unique_ptr<Node>;

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

	constexpr std::vector<NodeUPtr> make_node_ptr_vector(const std::vector<NodePtr>& ptrs) {
		std::vector<NodeUPtr> result;
		result.reserve(ptrs.size());
		for (auto&& p : ptrs) {
			result.push_back(NodeUPtr(p));
		}
		return result;
	}

	struct IdentifierNode final : public Node {
		std::string _name;
		explicit IdentifierNode(std::string name)
			: _name(std::move(name)) {
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			stream << _name.c_str();
		})
	};

	struct StringNode final : public Node {
		std::string _name;
		explicit StringNode(std::string name)
			: _name(std::move(name)) {
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			stream << '"' << _name.c_str() << '"';
		})
	};

	struct AssignNode final : public Node {
		std::string _name;
		NodeUPtr _initializer;
		explicit AssignNode(NodePtr name, NodePtr init)
			: _initializer(std::move(init)) {
			if (name->is_type<IdentifierNode>()) {
				_name = cast_node_ptr<IdentifierNode>(name)._name;
			}
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			stream << _name.c_str() << " = ";
			_initializer->print(stream);
		})
	};

	struct ListNode final : public Node {
		std::vector<NodeUPtr> _statements;
		explicit ListNode(std::vector<NodePtr> statements = std::vector<NodePtr> {})
			: _statements(make_node_ptr_vector(statements)) {
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			stream << '{';
			for (int i = 0; i < _statements.size(); i++) {
				auto& statement = _statements[i];
				statement->print(stream);
				if (i + 1 != _statements.size())
					stream << ' ';
			}
			stream << '}';
		})
	};

	struct FileNode final : public Node {
		std::vector<NodeUPtr> _statements;
		FileNode() {}
		explicit FileNode(std::vector<NodePtr> statements)
			: _statements(make_node_ptr_vector(statements)) {
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			for (auto& statement : _statements) {
				statement->print(stream);
				stream << "\n===========\n";
			}
		})
	};

	struct EventNode final : public Node {
		enum class Type {
			Country,
			Province
		} _type;
		std::vector<NodeUPtr> _statements;
		explicit EventNode(Type type, std::vector<NodePtr> statements)
			: _type(type),
			  _statements(make_node_ptr_vector(statements)) {
		}

		OVDL_TYPE_DEFINE_SELF;
		OVDL_RT_TYPE_DEF;

		OVDL_PRINT_FUNC_DEF({
			switch (_type) {
				case Type::Country: stream << "country_event"; break;
				case Type::Province: stream << "province_event"; break;
			}
			stream << " = {";
			for (auto& statement : _statements) {
				statement->print(stream);
			}
			stream << "}";
		})
	};
}

#undef OVDL_PRINT_FUNC_DECL
#undef OVDL_PRINT_FUNC_DEF
#undef OVDL_TYPE_DEFINE_SELF