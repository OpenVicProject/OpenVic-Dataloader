#pragma once

#include <lexy/dsl.hpp>

#include "SimpleGrammar.hpp"
#include "detail/LexyLitRange.hpp"

namespace ovdl::v2script::lua::grammar {
	struct ParseOptions {
	};

	template<ParseOptions Options>
	struct StatementListBlock;

	static constexpr auto comment_specifier = LEXY_LIT("--") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	template<ParseOptions Options>
	struct Identifier {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::alpha_underscore, lexy::dsl::ascii::alpha_digit_underscore);
		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto lexeme) {
				std::string str(lexeme.data(), lexeme.size());
				return ast::make_node_ptr<ast::IdentifierNode>(ast::NodeLocation { lexeme.begin(), lexeme.end() }, LEXY_MOV(str));
			});
	};

	template<ParseOptions Options>
	struct Value {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::digit / lexy::dsl::lit_c<'.'> / lexy::dsl::lit_c<'-'>);
		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto lexeme) {
				std::string str(lexeme.data(), lexeme.size());
				return ast::make_node_ptr<ast::IdentifierNode>(ast::NodeLocation { lexeme.begin(), lexeme.end() }, LEXY_MOV(str));
			});
	};

	template<ParseOptions Options>
	struct String {
		static constexpr auto rule = [] {
			// Arbitrary code points that aren't control characters.
			auto c = ovdl::detail::lexydsl::make_range<0x20, 0xFF>() - lexy::dsl::ascii::control;

			return lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'"'>))(c) | lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'\''>))(c);
		}();

		static constexpr auto value =
			lexy::as_string<std::string> >>
			lexy::callback<ast::NodePtr>(
				[](const char* begin, auto&& str, const char* end) {
					return ast::make_node_ptr<ast::StringNode>(ast::NodeLocation::make_from(begin, end), LEXY_MOV(str));
				});
	};

	template<ParseOptions Options>
	struct Expression {
		static constexpr auto rule = lexy::dsl::p<Value<Options>> | lexy::dsl::p<String<Options>>;
		static constexpr auto value = lexy::forward<ast::NodePtr>;
	};

	template<ParseOptions Options>
	struct AssignmentStatement {
		static constexpr auto rule =
			lexy::dsl::position(lexy::dsl::p<Identifier<Options>>) >>
			lexy::dsl::equal_sign >>
			(lexy::dsl::p<Expression<Options>> | lexy::dsl::recurse_branch<StatementListBlock<Options>>);

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](const char* pos, auto name, lexy::nullopt = {}) {
				return LEXY_MOV(name);
			},
			[](auto name, lexy::nullopt = {}) {
				return LEXY_MOV(name);
			},
			[](const char* pos, auto name, auto&& initalizer) {
				return ast::make_node_ptr<ast::AssignNode>(pos, LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	template<ParseOptions Options>
	struct StatementListBlock {
		static constexpr auto rule =
			lexy::dsl::position(lexy::dsl::curly_bracketed.open()) >>
			lexy::dsl::opt(
				lexy::dsl::list(
					lexy::dsl::recurse_branch<AssignmentStatement<Options>>,
					lexy::dsl::trailing_sep(lexy::dsl::lit_c<','>))) >>
			lexy::dsl::position(lexy::dsl::curly_bracketed.close());

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](const char* begin, lexy::nullopt, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end));
				},
				[](const char* begin, auto&& list, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end), LEXY_MOV(list));
				},
				[](const char* begin, auto& list, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end), list);
				});
	};

	template<ParseOptions Options = ParseOptions {}>
	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = ovdl::v2script::grammar::whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<AssignmentStatement<Options>>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};
}