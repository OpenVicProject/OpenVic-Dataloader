#include <filesystem>
#include <fstream>
#include <string_view>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

#include <dryad/node.hpp>

#include "Helper.hpp"
#include <detail/NullBuff.hpp>
#include <range/v3/iterator/operations.hpp>
#include <range/v3/view/enumerate.hpp>
#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace v2script;
using namespace std::string_view_literals;

static constexpr auto simple_buffer = "a = b"sv;
static constexpr auto simple_path = "file.txt"sv;

static void SetupFile(std::string_view path) {
	std::ofstream stream(path.data());
	stream << simple_buffer << std::flush;
}

#define CHECK_PARSE(...)                               \
	CHECK_OR_RETURN(parser.get_errors().empty());      \
	CHECK_OR_RETURN(parser.simple_parse(__VA_ARGS__)); \
	CHECK_OR_RETURN(parser.get_errors().empty())

TEST_CASE("V2Script Memory Buffer (data, size) Simple Parse", "[v2script-memory-simple-parse][buffer][data-size]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(simple_buffer.data(), simple_buffer.size());

	CHECK_PARSE();
}

TEST_CASE("V2Script Memory Buffer (begin, end) Simple Parse", "[v2script-memory-simple-parse][buffer][begin-end]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(simple_buffer.data(), simple_buffer.data() + simple_buffer.size());

	CHECK_PARSE();
}

TEST_CASE("V2Script Memory Buffer String Simple Parse", "[v2script-memory-simple-parse][buffer][string]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_string(simple_buffer);

	CHECK_PARSE();
}

TEST_CASE("V2Script Buffer nullptr Simple Parse", "[v2script-memory-simple-parse][buffer][nullptr]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(nullptr, std::size_t { 0 });

	CHECK_PARSE();
}

TEST_CASE("V2Script File (const char*) Simple Parse", "[v2script-file-simple-parse][char-ptr]") {
	SetupFile(simple_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(simple_path.data());

	std::filesystem::remove(simple_path);

	CHECK_PARSE();
}

TEST_CASE("V2Script File (filesystem::path) Simple Parse", "[v2script-file-simple-parse][filesystem-path]") {
	SetupFile(simple_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::filesystem::path(simple_path));

	std::filesystem::remove(simple_path);

	CHECK_PARSE();
}

TEST_CASE("V2Script File (HasCstr) Simple Parse", "[v2script-file-simple-parse][has-cstr]") {
	SetupFile(simple_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::string { simple_path });

	std::filesystem::remove(simple_path);

	CHECK_PARSE();
}

TEST_CASE("V2Script File (const char*) Handle Empty Path String Parse", "[v2script-file-parse][handle-string][char-ptr][empty-path]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_file("");

	CHECK_OR_RETURN(!parser.get_errors().empty());
}

TEST_CASE("V2Script File (const char*) Handle Non-existent Path String Parse", "[v2script-file-parse][handle-string][char-ptr][nonexistent-path]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_file("./Idontexist");

	CHECK_OR_RETURN(!parser.get_errors().empty());
}

TEST_CASE("V2Script Identifier Simple Parse", "[v2script-id-simple-parse]") {
	Parser parser(ovdl::detail::cnull);

	SECTION("a = b") {
		static constexpr auto buffer = "a = b"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = statements.front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::IdentifierValue>(assign->right());
		CHECK_IF(right) {
			CHECK(parser.value(right) == "b"sv);
		}
	}

	SECTION("a b c d") {
		static constexpr auto buffer = "a b c d"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 4);

		for (const auto [statement_index, statement] : statements | ranges::views::enumerate) {
			CHECK_OR_CONTINUE(statement);

			const auto* value_statement = dryad::node_try_cast<ast::ValueStatement>(statement);
			CHECK_OR_CONTINUE(value_statement);

			const auto* value = dryad::node_try_cast<ast::IdentifierValue>(value_statement->value());
			CHECK_OR_CONTINUE(value);
			switch (statement_index) {
				case 0: CHECK_OR_CONTINUE(parser.value(value) == "a"sv); break;
				case 1: CHECK_OR_CONTINUE(parser.value(value) == "b"sv); break;
				case 2: CHECK_OR_CONTINUE(parser.value(value) == "c"sv); break;
				case 3: CHECK_OR_CONTINUE(parser.value(value) == "d"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION("a = { a = b }") {
		static constexpr auto buffer = "a = { a = b }"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = statements.front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::ListValue>(assign->right());
		CHECK_IF(right) {
			const auto inner_statements = right->statements();
			CHECK_FALSE(inner_statements.empty());
			CHECK(ranges::distance(inner_statements) == 1);

			const ast::Statement* inner_statement = inner_statements.front();
			CHECK(inner_statement);

			const auto* inner_assign = dryad::node_try_cast<ast::AssignStatement>(inner_statement);
			CHECK(inner_assign);
			CHECK(inner_assign->left());
			CHECK(inner_assign->right());

			const auto* inner_left = dryad::node_try_cast<ast::IdentifierValue>(inner_assign->left());
			CHECK_IF(inner_left) {
				CHECK(parser.value(inner_left) == "a"sv);
			}

			const auto* inner_right = dryad::node_try_cast<ast::IdentifierValue>(inner_assign->right());
			CHECK_IF(inner_right) {
				CHECK(parser.value(inner_right) == "b"sv);
			}
		}
	}

	SECTION("a = { { a } }") {
		static constexpr auto buffer = "a = { { a } }"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = statements.front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::ListValue>(assign->right());
		CHECK_IF(right) {
			const auto inner_statements = right->statements();
			CHECK_FALSE(inner_statements.empty());
			CHECK(ranges::distance(inner_statements) == 1);

			const ast::Statement* inner_statement = inner_statements.front();
			CHECK(inner_statement);

			const auto* value_statement = dryad::node_try_cast<ast::ValueStatement>(inner_statement);
			CHECK(value_statement);

			const auto* list_value = dryad::node_try_cast<ast::ListValue>(value_statement->value());
			CHECK(list_value);

			const auto list_statements = list_value->statements();
			CHECK_FALSE(list_statements.empty());
			CHECK(ranges::distance(list_statements) == 1);

			const auto* inner_value_statement = dryad::node_try_cast<ast::ValueStatement>(list_statements.front());
			CHECK(inner_value_statement);

			const auto* id_value = dryad::node_try_cast<ast::IdentifierValue>(inner_value_statement->value());
			CHECK(id_value);
			CHECK(parser.value(id_value) == "a"sv);
		}
	}
}

TEST_CASE("V2Script String Simple Parse", "[v2script-id-simple-parse]") {
	Parser parser(ovdl::detail::cnull);

	SECTION("a = \"b\"") {
		static constexpr auto buffer = "a = \"b\""sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = statements.front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::StringValue>(assign->right());
		CHECK_IF(right) {
			CHECK(parser.value(right) == "b"sv);
		}
	}

	SECTION("\"a\" \"b\" \"c\" \"d\"") {
		static constexpr auto buffer = "\"a\" \"b\" \"c\" \"d\""sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 4);

		for (const auto [statement_index, statement] : statements | ranges::views::enumerate) {
			CHECK_OR_CONTINUE(statement);

			const auto* value_statement = dryad::node_try_cast<ast::ValueStatement>(statement);
			CHECK_OR_CONTINUE(value_statement);

			const auto* value = dryad::node_try_cast<ast::StringValue>(value_statement->value());
			CHECK_OR_CONTINUE(value);
			switch (statement_index) {
				case 0: CHECK_OR_CONTINUE(parser.value(value) == "a"sv); break;
				case 1: CHECK_OR_CONTINUE(parser.value(value) == "b"sv); break;
				case 2: CHECK_OR_CONTINUE(parser.value(value) == "c"sv); break;
				case 3: CHECK_OR_CONTINUE(parser.value(value) == "d"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION("a = { a = \"b\" }") {
		static constexpr auto buffer = "a = { a = \"b\" }"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = file_tree->statements().front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::ListValue>(assign->right());
		CHECK_IF(right) {
			const auto inner_statements = right->statements();
			CHECK_FALSE(inner_statements.empty());
			CHECK(ranges::distance(inner_statements) == 1);

			const ast::Statement* inner_statement = inner_statements.front();
			CHECK(inner_statement);

			const auto* inner_assign = dryad::node_try_cast<ast::AssignStatement>(inner_statement);
			CHECK(inner_assign);
			CHECK(inner_assign->left());
			CHECK(inner_assign->right());

			const auto* inner_left = dryad::node_try_cast<ast::IdentifierValue>(inner_assign->left());
			CHECK_IF(inner_left) {
				CHECK(parser.value(inner_left) == "a"sv);
			}

			const auto* inner_right = dryad::node_try_cast<ast::StringValue>(inner_assign->right());
			CHECK_IF(inner_right) {
				CHECK(parser.value(inner_right) == "b"sv);
			}
		}
	}

	SECTION("a = { { \"a\" } }") {
		static constexpr auto buffer = "a = { { \"a\" } }"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const ast::FileTree* file_tree = parser.get_file_node();
		CHECK(file_tree);

		const auto statements = file_tree->statements();
		CHECK_FALSE(statements.empty());
		CHECK(ranges::distance(statements) == 1);

		const ast::Statement* statement = statements.front();
		CHECK(statement);

		const auto* assign = dryad::node_try_cast<ast::AssignStatement>(statement);
		CHECK(assign);
		CHECK(assign->left());
		CHECK(assign->right());

		const auto* left = dryad::node_try_cast<ast::IdentifierValue>(assign->left());
		CHECK_IF(left) {
			CHECK(parser.value(left) == "a"sv);
		}

		const auto* right = dryad::node_try_cast<ast::ListValue>(assign->right());
		CHECK_IF(right) {
			const auto inner_statements = right->statements();
			CHECK_FALSE(inner_statements.empty());
			CHECK(ranges::distance(inner_statements) == 1);

			const ast::Statement* inner_statement = inner_statements.front();
			CHECK(inner_statement);

			const auto* value_statement = dryad::node_try_cast<ast::ValueStatement>(inner_statement);
			CHECK(value_statement);

			const auto* list_value = dryad::node_try_cast<ast::ListValue>(value_statement->value());
			CHECK(list_value);

			const auto list_statements = list_value->statements();
			CHECK_FALSE(list_statements.empty());
			CHECK(ranges::distance(list_statements) == 1);

			const auto* inner_value_statement = dryad::node_try_cast<ast::ValueStatement>(list_statements.front());
			CHECK_OR_RETURN(inner_value_statement);

			const auto* str_value = dryad::node_try_cast<ast::StringValue>(inner_value_statement->value());
			CHECK_OR_RETURN(str_value);
			CHECK(parser.value(str_value) == "a"sv);
		}
	}
}