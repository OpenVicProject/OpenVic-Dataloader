#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node_map.hpp>
#include <dryad/tree.hpp>

#include "Helper.hpp"
#include <range/v3/iterator/operations.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/zip.hpp>
#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace ovdl::v2script::ast;
using namespace std::string_view_literals;

struct Ast : SymbolIntern {
	dryad::tree<FileTree> tree;
	dryad::node_map<const Node, NodeLocation> map;
	symbol_interner_type symbol_interner;

	template<typename T, typename... Args>
	T* create(Args&&... args) {
		auto node = tree.template create<T>(DRYAD_FWD(args)...);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_intern(std::string_view view, Args&&... args) {
		auto intern = symbol_interner.intern(view.data(), view.size());
		auto node = tree.template create<T>(intern, DRYAD_FWD(args)...);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_loc(NodeLocation loc, Args&&... args) {
		auto node = tree.template create<T>(DRYAD_FWD(args)...);
		set_location(node, loc);
		return node;
	}

	template<typename T, typename... Args>
	T* create_with_loc_and_intern(NodeLocation loc, std::string_view view, Args&&... args) {
		auto intern = symbol_interner.intern(view.data(), view.size());
		auto node = tree.template create<T>(intern, DRYAD_FWD(args)...);
		set_location(node, loc);
		return node;
	}

	void set_location(const Node* n, NodeLocation loc) {
		map.insert(n, loc);
	}

	NodeLocation location_of(const Node* n) const {
		auto result = map.lookup(n);
		if (result == nullptr)
			return {};
		return *result;
	}
};

TEST_CASE("V2Script Nodes", "[v2script-nodes]") {
	Ast ast;

	auto* id = ast.create_with_intern<IdentifierValue>("id");
	CHECK_IF(id) {
		CHECK(id->kind() == NodeKind::IdentifierValue);
		CHECK(id->value().view() == "id"sv);
	}

	auto* str = ast.create_with_intern<StringValue>("str");
	CHECK_IF(str) {
		CHECK(str->kind() == NodeKind::StringValue);
		CHECK(str->value().view() == "str"sv);
	}

	auto* list = ast.create<ListValue>();
	CHECK_IF(list) {
		CHECK(list->kind() == NodeKind::ListValue);
	}

	auto* null = ast.create<NullValue>();
	CHECK_IF(null) {
		CHECK(null->kind() == NodeKind::NullValue);
	}

	auto* event = ast.create<EventStatement>(false, list);
	CHECK_IF(event) {
		CHECK(event->kind() == NodeKind::EventStatement);
		CHECK(!event->is_province_event());
		CHECK(event->right() == list);
	}

	auto* assign = ast.create<AssignStatement>(id, str);
	CHECK_IF(assign) {
		CHECK(assign->kind() == NodeKind::AssignStatement);
		CHECK(assign->left() == id);
		CHECK(assign->right() == str);
	}

	auto* value_statement = ast.create<ValueStatement>(null);
	CHECK_IF(value_statement) {
		CHECK(value_statement->kind() == NodeKind::ValueStatement);
		CHECK(value_statement->value() == null);
	}

	if (!assign || !value_statement || !event || !null || !list || !str || !id) {
		return;
	}

	StatementList statement_list {};
	statement_list.push_back(assign);
	statement_list.push_back(value_statement);
	statement_list.push_back(event);
	CHECK_FALSE(statement_list.empty());
	CHECK(ranges::distance(statement_list) == 3);

	for (const auto [statement_list_index, statement] : statement_list | ranges::views::enumerate) {
		CAPTURE(statement_list_index);
		CHECK_OR_CONTINUE(statement);
		switch (statement_list_index) {
			case 0: CHECK_OR_CONTINUE(statement == assign); break;
			case 1: CHECK_OR_CONTINUE(statement == value_statement); break;
			case 2: CHECK_OR_CONTINUE(statement == event); break;
			default: CHECK_OR_CONTINUE(false); break;
		}
	}

	auto* file_tree = ast.create<FileTree>(statement_list);
	CHECK_IF(file_tree) {
		CHECK(file_tree->kind() == NodeKind::FileTree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 3);

		for (const auto [statement, list_statement] : ranges::views::zip(statements, statement_list)) {
			CHECK_OR_CONTINUE(list_statement);
			CHECK_OR_CONTINUE(statement);
			CHECK_OR_CONTINUE(statement == list_statement);
		}
	}

	ast.tree.set_root(file_tree);
	CHECK(ast.tree.has_root());
	CHECK(ast.tree.root() == file_tree);

	ast.tree.clear();
	CHECK_FALSE(ast.tree.has_root());
	CHECK(ast.tree.root() != file_tree);
}

TEST_CASE("V2Script Nodes Location", "[v2script-nodes-location]") {
	Ast ast;

	constexpr auto fake_buffer = "id"sv;

	auto* id = ast.create_with_loc_and_intern<IdentifierValue>(NodeLocation::make_from(&fake_buffer[0], &fake_buffer[1]), "id");

	CHECK_IF(id) {
		CHECK(id->value().view() == "id"sv);

		auto location = ast.location_of(id);
		CHECK_FALSE(location.is_synthesized());
		CHECK(location.begin() == &fake_buffer[0]);
		CHECK(location.end() - 1 == &fake_buffer[1]);
	}
}