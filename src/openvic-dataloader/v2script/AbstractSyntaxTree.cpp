#include <iomanip>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <stddef.h>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

using namespace ovdl::v2script::ast;

void ovdl::v2script::ast::copy_into_node_ptr_vector(const std::vector<NodePtr>& source, std::vector<NodeUPtr>& dest) {
	dest.clear();
	dest.reserve(source.size());
	for (auto&& p : source) {
		dest.push_back(NodeUPtr { p });
	}
}

AbstractStringNode::AbstractStringNode(std::string&& name) : _name(std::move(name)) {}
std::ostream& AbstractStringNode::print(std::ostream& stream, size_t indent) const {
	return stream << _name;
}

#define OVDL_AST_STRING_NODE_DEF(NAME, ...)                                 \
	NAME::NAME(std::string&& name) : AbstractStringNode(std::move(name)) {} \
	std::ostream& NAME::print(std::ostream& stream, size_t indent) const __VA_ARGS__

OVDL_AST_STRING_NODE_DEF(IdentifierNode, {
	return stream << _name;
});

OVDL_AST_STRING_NODE_DEF(StringNode, {
	return stream << '"' << _name << '"';
});

OVDL_AST_STRING_NODE_DEF(FactorNode, {
	return stream << "factor = " << _name;
});

OVDL_AST_STRING_NODE_DEF(MonthNode, {
	return stream << "months = " << _name;
});

OVDL_AST_STRING_NODE_DEF(NameNode, {
	return stream << "name = " << _name;
});

OVDL_AST_STRING_NODE_DEF(FireOnlyNode, {
	return stream << "fire_only_once = " << _name;
});

OVDL_AST_STRING_NODE_DEF(IdNode, {
	return stream << "id = " << _name;
});

OVDL_AST_STRING_NODE_DEF(TitleNode, {
	return stream << "title = " << _name;
});

OVDL_AST_STRING_NODE_DEF(DescNode, {
	return stream << "desc = " << _name;
});

OVDL_AST_STRING_NODE_DEF(PictureNode, {
	return stream << "picture = " << _name;
});

OVDL_AST_STRING_NODE_DEF(IsTriggeredNode, {
	return stream << "is_triggered_only = " << _name;
});

#undef OVDL_AST_STRING_NODE_DEF

AssignNode::AssignNode(NodeCPtr name, NodePtr init)
	: _initializer(std::move(init)) {
	if (name->is_type<IdentifierNode>()) {
		_name = cast_node_cptr<IdentifierNode>(name)._name;
	}
}

std::ostream& Node::print_ptr(std::ostream& stream, NodeCPtr node, size_t indent) {
	return node != nullptr ? node->print(stream, indent) : stream << "<NULL>";
}

static std::ostream& print_newline_indent(std::ostream& stream, size_t indent) {
	return stream << "\n"
				  << std::setw(indent) << std::setfill('\t') << "";
}

/* Starts with a newline and ends at the end of a line, and so
 * should be followed by a call to print_newline_indent.
 */
static std::ostream& print_nodeuptr_vector(const std::vector<NodeUPtr>& nodes,
	std::ostream& stream, size_t indent) {
	for (NodeUPtr const& node : nodes) {
		print_newline_indent(stream, indent);
		Node::print_ptr(stream, node.get(), indent);
	}
	return stream;
}

AbstractListNode::AbstractListNode(const std::vector<NodePtr>& statements) {
	copy_into_node_ptr_vector(statements, _statements);
}
std::ostream& AbstractListNode::print(std::ostream& stream, size_t indent) const {
	stream << '{';
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << "}";
}

#define OVDL_AST_LIST_NODE_DEF(NAME, ...)                                                \
	NAME::NAME(const std::vector<NodePtr>& statements) : AbstractListNode(statements) {} \
	std::ostream& NAME::print(std::ostream& stream, size_t indent) const __VA_ARGS__

OVDL_AST_LIST_NODE_DEF(FileNode, {
	print_nodeuptr_vector(_statements, stream, indent);
	return print_newline_indent(stream, indent);
});

OVDL_AST_LIST_NODE_DEF(ListNode, {
	stream << '{';
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << "}";
});

OVDL_AST_LIST_NODE_DEF(ModifierNode, {
	stream << "modifier = {";
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
});

OVDL_AST_LIST_NODE_DEF(MtthNode, {
	stream << "mean_time_to_happen = {";
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
});

OVDL_AST_LIST_NODE_DEF(EventOptionNode, {
	stream << "option = {";
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
});

OVDL_AST_LIST_NODE_DEF(BehaviorListNode, {
	stream << "ai_chance = {"; // may be ai_chance or ai_will_do
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
});

OVDL_AST_LIST_NODE_DEF(DecisionListNode, {
	stream << "political_decisions = {";
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
});

#undef OVDL_AST_LIST_NODE_DEF

EventNode::EventNode(Type type, const std::vector<NodePtr>& statements) : _type(type) {
	copy_into_node_ptr_vector(statements, _statements);
}
std::ostream& EventNode::print(std::ostream& stream, size_t indent) const {
	switch (_type) {
		case Type::Country: stream << "country_event = "; break;
		case Type::Province: stream << "province_event = "; break;
	}
	stream << '{';
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
}

DecisionNode::DecisionNode(NodePtr name, const std::vector<NodePtr>& statements) : _name(std::move(name)) {
	copy_into_node_ptr_vector(statements, _statements);
}
std::ostream& DecisionNode::print(std::ostream& stream, size_t indent) const {
	print_ptr(stream, _name.get(), indent) << " = {";
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << '}';
}

ExecutionNode::ExecutionNode(Type type, NodePtr name, NodePtr init) : _type(type),
																	  _name(std::move(name)),
																	  _initializer(std::move(init)) {
}
std::ostream& ExecutionNode::print(std::ostream& stream, size_t indent) const {
	print_ptr(stream, _name.get(), indent) << " = ";
	if (_initializer) {
		Node::print_ptr(stream, _initializer.get(), indent + 1);
	}
	return stream;
}

ExecutionListNode::ExecutionListNode(ExecutionNode::Type type, const std::vector<NodePtr>& statements) : _type(type) {
	copy_into_node_ptr_vector(statements, _statements);
}
std::ostream& ExecutionListNode::print(std::ostream& stream, size_t indent) const {
	// Only way to make a valid declared parsable file
	stream << "{ ";
	switch (_type) {
		case ExecutionNode::Type::Effect: stream << "effect = {"; break;
		case ExecutionNode::Type::Trigger: stream << "trigger = {"; break;
	}
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << "}}";
}

Node::operator std::string() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream& AssignNode::print(std::ostream& stream, size_t indent) const {
	stream << _name << " = ";
	return Node::print_ptr(stream, _initializer.get(), indent);
}
