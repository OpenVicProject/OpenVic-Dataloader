#pragma once

#include <memory>
#include <string>
#include <vector>

#include "detail/LexyLitRange.hpp"
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

// Grammar Definitions //
namespace ovdl::v2script::grammar {
	struct StatementListBlock;

	static constexpr auto whitespace_specifier = lexy::dsl::ascii::blank / lexy::dsl::ascii::newline;
	static constexpr auto comment_specifier = LEXY_LIT("#") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	static constexpr auto data_specifier =
		lexy::dsl::ascii::alpha_digit_underscore /
		LEXY_ASCII_ONE_OF("%&'") / lexy::dsl::lit_c<0x2B> / LEXY_ASCII_ONE_OF("-.") /
		lexy::dsl::ascii::digit / lexy::dsl::lit_c<0x3A> /
		lexy::dsl::lit_c<0x40> / lexy::dsl::ascii::upper / lexy::dsl::lit_c<0x5F> /
		lexy::dsl::ascii::lower / lexy::dsl::lit_b<0x8A> / lexy::dsl::lit_b<0x8C> / lexy::dsl::lit_b<0x8E> /
		lexy::dsl::lit_b<0x92> / lexy::dsl::lit_b<0x97> / lexy::dsl::lit_b<0x9A> / lexy::dsl::lit_b<0x9C> / lexy::dsl::lit_b<0x9E> / lexy::dsl::lit_b<0x9F> /
		lexy::dsl::lit_b<0xC0> /
		ovdl::detail::lexydsl::make_range<0xC0, 0xD6>() / ovdl::detail::lexydsl::make_range<0xD8, 0xF6>() / ovdl::detail::lexydsl::make_range<0xF8, 0xFF>();

	static constexpr auto data_char_class = LEXY_CHAR_CLASS("DataSpecifier", data_specifier);

	struct Identifier {
		static constexpr auto rule = lexy::dsl::identifier(data_char_class);
		static constexpr auto value = lexy::as_string<std::string> | lexy::new_<ast::IdentifierNode, ast::NodePtr>;
	};

	struct StringExpression {
		static constexpr auto escaped_symbols = lexy::symbol_table<char> //
													.map<'"'>('"')
													.map<'\''>('\'')
													.map<'\\'>('\\')
													.map<'/'>('/')
													.map<'b'>('\b')
													.map<'f'>('\f')
													.map<'n'>('\n')
													.map<'r'>('\r')
													.map<'t'>('\t');
		static constexpr auto rule = [] {
			// Arbitrary code points that aren't control characters.
			auto c = ovdl::detail::lexydsl::make_range<0x20, 0xFF>() - lexy::dsl::ascii::control;

			// Escape sequences start with a backlash.
			// They either map one of the symbols,
			// or a Unicode code point of the form uXXXX.
			auto escape = lexy::dsl::backslash_escape //
							  .symbol<escaped_symbols>();
			return lexy::dsl::quoted(c, escape);
		}();

		static constexpr auto value = lexy::as_string<std::string> >> lexy::new_<ast::StringNode, ast::NodePtr>;
	};

	struct SimpleAssignmentStatement {
		static constexpr auto rule =
			lexy::dsl::p<Identifier> >>
			lexy::dsl::equal_sign +
				(lexy::dsl::p<Identifier> | lexy::dsl::p<StringExpression> | lexy::dsl::recurse_branch<StatementListBlock>);

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto name, auto&& initalizer) {
				return make_node_ptr<ast::AssignNode>(LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	struct AssignmentStatement {
		static constexpr auto rule =
			lexy::dsl::p<Identifier> >>
				(lexy::dsl::equal_sign >>
						(lexy::dsl::p<Identifier> | lexy::dsl::p<StringExpression> | lexy::dsl::recurse_branch<StatementListBlock>) |
					lexy::dsl::else_ >> lexy::dsl::return_) |
			lexy::dsl::p<StringExpression>;

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto name, lexy::nullopt = {}) {
				return LEXY_MOV(name);
			},
			[](auto name, auto&& initalizer) {
				return make_node_ptr<ast::AssignNode>(LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	struct StatementListBlock {
		static constexpr auto rule =
			lexy::dsl::curly_bracketed.open() >>
			lexy::dsl::opt(lexy::dsl::list(lexy::dsl::p<AssignmentStatement>)) + lexy::dsl::opt(lexy::dsl::semicolon) +
				lexy::dsl::curly_bracketed.close();

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](lexy::nullopt = {}, lexy::nullopt = {}) {
					return ast::make_node_ptr<ast::ListNode>();
				},
				[](auto&& list, lexy::nullopt = {}) {
					return make_node_ptr<ast::ListNode>(LEXY_MOV(list));
				},
				[](auto& list) {
					return make_node_ptr<ast::ListNode>(list);
				});
	};

	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::terminator(lexy::dsl::eof).list(lexy::dsl::p<AssignmentStatement>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};
}
