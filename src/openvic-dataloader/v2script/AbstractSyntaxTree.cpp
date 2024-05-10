#include <stddef.h>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/dsl/option.hpp>
#include <lexy/input_location.hpp>

#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <range/v3/view/drop.hpp>

using namespace ovdl::v2script::ast;

ListValue::ListValue(dryad::node_ctor ctor, StatementList statements)
	: node_base(ctor) {
	insert_child_list_after(nullptr, statements);
	if (statements.empty()) {
		_last_statement = nullptr;
	} else {
		_last_statement = statements.back();
	}
}

FileTree::FileTree(dryad::node_ctor ctor, StatementList statements) : node_base(ctor) {
	insert_child_list_after(nullptr, statements);
	if (statements.empty()) {
		_last_node = nullptr;
	} else {
		_last_node = statements.back();
	}
}

// static void _handle_string_characters(std::string& string, bool allow_newline) {
// 	size_t position = 0;
// 	for (auto& c : string) {
// 		switch (c) {
// 			case '\r':
// 			case '\n':
// 				if (allow_newline) goto END_LOOP;
// 				c = ' ';
// 				break;
// 			default: break;
// 		}
// 	END_LOOP:
// 		position++;
// 	}
// }

std::string AbstractSyntaxTree::make_list_visualizer() const {
	const int INDENT_SIZE = 2;

	std::string result;
	unsigned int level = 0;

	for (auto [event, node] : dryad::traverse(_tree)) {
		if (event == dryad::traverse_event::exit) {
			--level;
			continue;
		}

		result.append(INDENT_SIZE * level, ' ');
		result.append(fmt::format("- {}: ", get_kind_name(node->kind())));

		dryad::visit_node(
			node,
			[&](const FlatValue* value) {
				result.append(value->value(_symbol_interner));
			},
			[&](const ListValue* value) {
			},
			[&](const NullValue* value) {
			},
			[&](const EventStatement* statement) {
				result.append(statement->is_province_event() ? "province_event" : "country_event");
			},
			[&](const AssignStatement* statement) {
			},
			[&](const FileTree* tree) {
			});

		result.append(1, '\n');

		if (event == dryad::traverse_event::enter)
			++level;
	}

	return result;
}

std::string AbstractSyntaxTree::make_native_visualizer() const {
	constexpr int INDENT_SIZE = 2;

	std::string result;
	unsigned int level = 0;

	dryad::visit_tree(
		_tree,
		[&](const IdentifierValue* value) {
			result.append(value->value(_symbol_interner));
		},
		[&](const StringValue* value) {
			result.append(1, '"').append(value->value(_symbol_interner)).append(1, '"');
		},
		[&](dryad::child_visitor<NodeKind> visitor, const ValueStatement* statement) {
			visitor(statement->value());
		},
		[&](dryad::child_visitor<NodeKind> visitor, const AssignStatement* statement) {
			visitor(statement->left());
			if (statement->right()->kind() != NodeKind::NullValue) {
				result.append(" = ");
				visitor(statement->right());
			}
		},
		[&](dryad::child_visitor<NodeKind> visitor, const ListValue* value) {
			result.append(1, '{');
			level++;
			for (const auto& statement : value->statements()) {
				result
					.append(1, '\n')
					.append(INDENT_SIZE * level, ' ');
				visitor(statement);
			}
			level--;
			result
				.append(1, '\n')
				.append(INDENT_SIZE * level, ' ')
				.append(1, '}');
		},
		[&](dryad::child_visitor<NodeKind> visitor, const EventStatement* statement) {
			result.append(statement->is_province_event() ? "province_event" : "country_event");
			if (statement->right()->kind() != NodeKind::NullValue) {
				result.append(" = ");
				visitor(statement->right());
			}
		},
		[&](dryad::child_visitor<NodeKind> visitor, const FileTree* value) {
			auto statements = value->statements();
			visitor(*statements.begin());

			for (const auto& statement : statements | ranges::views::drop(1)) {
				result
					.append(1, '\n')
					.append(INDENT_SIZE * level, ' ');
				visitor(statement);
			}
		});

	return result;
}