#pragma once

#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <dryad/_detail/assert.hpp>
#include <dryad/_detail/config.hpp>
#include <dryad/abstract_node.hpp>
#include <dryad/node.hpp>
#include <dryad/symbol.hpp>
#include <dryad/tree.hpp>

namespace ovdl::v2script::ast {
	enum class NodeKind {
		FileTree,

		// FlatValues //
		IdentifierValue, // straight_identifier_value
		StringValue,	 // "plain string value"

		FirstFlatValue = IdentifierValue,
		LastFlatValue = StringValue,

		// Values //
		ListValue, // { <StatementList> }
		NullValue,

		FirstValue = FirstFlatValue,
		LastValue = NullValue,

		// Statements //
		EventStatement,	 // (country_event|province_event) = { id = <FlatValue> ... }
		AssignStatement, // <IdentifierValue> = <Value>
		ValueStatement,	 // <Value>

		FirstStatement = EventStatement,
		LastStatement = ValueStatement,
	};

	constexpr std::string_view get_kind_name(NodeKind kind) {
		switch (kind) {
			using enum NodeKind;
			case FileTree: return "file tree";
			case IdentifierValue: return "identifier value";
			case StringValue: return "string value";
			case ListValue: return "list value";
			case NullValue: return "null value";
			case EventStatement: return "event statement";
			case AssignStatement: return "assign statement";
			case ValueStatement: return "value statement";
			default: detail::unreachable();
		}
	}

	using Node = dryad::node<NodeKind>;
	using NodeList = dryad::unlinked_node_list<Node>;

	struct Value;

	struct FlatValue;
	struct IdentifierValue;
	struct StringValue;

	struct ListValue;

	struct Statement;
	using StatementList = dryad::unlinked_node_list<Statement>;

	struct EventStatement;
	using EventStatementList = dryad::unlinked_node_list<EventStatement>;

	struct AssignStatement;
	using AssignStatementList = dryad::unlinked_node_list<AssignStatement>;

	struct Value : dryad::abstract_node_range<Node, NodeKind::FirstValue, NodeKind::LastValue> {
		DRYAD_ABSTRACT_NODE_CTOR(Value);
	};

	struct FlatValue : dryad::abstract_node_range<Value, NodeKind::FirstFlatValue, NodeKind::LastFlatValue> {
		SymbolIntern::symbol_type value() const {
			return _value;
		}

		const char* value(const SymbolIntern::symbol_interner_type& symbols) const {
			return _value.c_str(symbols);
		}

	protected:
		explicit FlatValue(dryad::node_ctor ctor, NodeKind kind, SymbolIntern::symbol_type value)
			: node_base(ctor, kind),
			  _value(value) {}

	protected:
		SymbolIntern::symbol_type _value;
	};

	struct IdentifierValue : dryad::basic_node<NodeKind::IdentifierValue, FlatValue> {
		explicit IdentifierValue(dryad::node_ctor ctor, SymbolIntern::symbol_type value) : node_base(ctor, value) {}
	};

	struct StringValue : dryad::basic_node<NodeKind::StringValue, FlatValue> {
		explicit StringValue(dryad::node_ctor ctor, SymbolIntern::symbol_type value) : node_base(ctor, value) {}
	};

	struct ListValue : dryad::basic_node<NodeKind::ListValue, dryad::container_node<Value>> {
		explicit ListValue(dryad::node_ctor ctor, StatementList statements);
		explicit ListValue(dryad::node_ctor ctor, AssignStatementList statements);

		explicit ListValue(dryad::node_ctor ctor) : ListValue(ctor, StatementList {}) {
		}

		DRYAD_CHILD_NODE_RANGE_GETTER(Statement, statements, nullptr, this->node_after(_last_statement));

	private:
		Node* _last_statement;
	};

	struct NullValue : dryad::basic_node<NodeKind::NullValue, Value> {
		explicit NullValue(dryad::node_ctor ctor) : node_base(ctor) {}
	};

	struct Statement : dryad::abstract_node_range<dryad::container_node<Node>, NodeKind::FirstStatement, NodeKind::LastStatement> {
		explicit Statement(dryad::node_ctor ctor, NodeKind kind, Value* right)
			: node_base(ctor, kind) {
			insert_child_after(nullptr, right);
		}

		explicit Statement(dryad::node_ctor ctor, NodeKind kind, Value* left, Value* right)
			: node_base(ctor, kind) {
			insert_child_after(nullptr, left);
			insert_child_after(left, right);
		}
	};

	struct EventStatement : dryad::basic_node<NodeKind::EventStatement, Statement> {
		explicit EventStatement(dryad::node_ctor ctor, bool is_province_event, ListValue* list)
			: basic_node(ctor, list),
			  _is_province_event(is_province_event) {
		}

		bool is_province_event() const { return _is_province_event; }

		DRYAD_CHILD_NODE_GETTER(Value, right, nullptr);

	private:
		bool _is_province_event;
	};

	struct AssignStatement : dryad::basic_node<NodeKind::AssignStatement, Statement> {
		explicit AssignStatement(dryad::node_ctor ctor, Value* left, Value* right)
			: node_base(ctor, left, right) {
		}
		DRYAD_CHILD_NODE_GETTER(Value, left, nullptr);
		DRYAD_CHILD_NODE_GETTER(Value, right, left());
	};

	struct ValueStatement : dryad::basic_node<NodeKind::ValueStatement, Statement> {
		explicit ValueStatement(dryad::node_ctor ctor, Value* value)
			: node_base(ctor, value) {
		}
		DRYAD_CHILD_NODE_GETTER(Value, value, nullptr);
	};

	struct FileTree : dryad::basic_node<NodeKind::FileTree, dryad::container_node<Node>> {
		explicit FileTree(dryad::node_ctor ctor, StatementList statements);
		explicit FileTree(dryad::node_ctor ctor, AssignStatementList statements);
		explicit FileTree(dryad::node_ctor ctor) : FileTree(ctor, StatementList {}) {
		}

		DRYAD_CHILD_NODE_RANGE_GETTER(Statement, statements, nullptr, this->node_after(_last_node));

	private:
		Node* _last_node;
	};
}