#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <iomanip>
#include <sstream>

using namespace ovdl::v2script::ast;

void ovdl::v2script::ast::copy_into_node_ptr_vector(const std::vector<NodePtr>& source, std::vector<NodeUPtr>& dest) {
	dest.clear();
	dest.reserve(source.size());
	for (auto&& p : source) {
		dest.push_back(NodeUPtr { p });
	}
}

IdentifierNode::IdentifierNode(std::string name)
	: _name(std::move(name)) {}

StringNode::StringNode(std::string name)
	: _name(std::move(name)) {}

AssignNode::AssignNode(NodeCPtr name, NodePtr init)
	: _initializer(std::move(init)) {
	if (name->is_type<IdentifierNode>()) {
		_name = cast_node_cptr<IdentifierNode>(name)._name;
	}
}

ListNode::ListNode(std::vector<NodePtr> statements) {
	copy_into_node_ptr_vector(statements, _statements);
}

FileNode::FileNode(std::vector<NodePtr> statements) {
	copy_into_node_ptr_vector(statements, _statements);
}

std::ostream& Node::print_ptr(std::ostream& stream, NodeCPtr node, size_t indent) {
	return node != nullptr ? node->print(stream, indent) : stream << "<NULL>";
}

static std::ostream& print_newline_indent(std::ostream& stream, size_t indent) {
	return stream << "\n" << std::setw(indent) << std::setfill('\t') << "";
}

/* Starts with a newline and ends at the end of a line, and so
 * should be followed by a call to print_newline_indent.
 */
static std::ostream& print_nodeuptr_vector(std::vector<NodeUPtr> const& nodes,
	std::ostream& stream, size_t indent) {
	for (NodeUPtr const& node : nodes) {
		print_newline_indent(stream, indent);
		Node::print_ptr(stream, node.get(), indent);
	}
	return stream;
}

Node::operator std::string() const {
	std::stringstream ss;
	ss << *this;
	return ss.str();
}

std::ostream& IdentifierNode::print(std::ostream& stream, size_t indent) const {
	return stream << _name;
}

std::ostream& StringNode::print(std::ostream& stream, size_t indent) const {
	return stream << '"' << _name << '"';
};

std::ostream& AssignNode::print(std::ostream& stream, size_t indent) const {
	stream << _name << " = ";
	return Node::print_ptr(stream, _initializer.get(), indent);
}

std::ostream& ListNode::print(std::ostream& stream, size_t indent) const {
	stream << '{';
	if (!_statements.empty()) {
		print_nodeuptr_vector(_statements, stream, indent + 1);
		print_newline_indent(stream, indent);
	}
	return stream << "}";
}

std::ostream& FileNode::print(std::ostream& stream, size_t indent) const {
	print_nodeuptr_vector(_statements, stream, indent);
	return print_newline_indent(stream, indent);
}
