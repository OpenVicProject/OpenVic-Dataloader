#pragma once

#include <string>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "detail/LexyLitRange.hpp"

// Grammar Definitions //
/* REQUIREMENTS:
 * DAT-626
 * DAT-627
 * DAT-628
 * DAT-636
 * DAT-641
 * DAT-642
 * DAT-643
 */
namespace ovdl::v2script::grammar {
	struct ParseOptions {
		/// @brief Makes string parsing avoid string escapes
		bool NoStringEscape;
	};

	static constexpr ParseOptions NoStringEscapeOption = ParseOptions { true };
	static constexpr ParseOptions StringEscapeOption = ParseOptions { false };

	template<ParseOptions Options>
	struct StatementListBlock;
	template<ParseOptions Options>
	struct AssignmentStatement;

	/* REQUIREMENTS: DAT-630 */
	static constexpr auto whitespace_specifier = lexy::dsl::ascii::blank / lexy::dsl::ascii::newline;
	/* REQUIREMENTS: DAT-631 */
	static constexpr auto comment_specifier = LEXY_LIT("#") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	/* REQUIREMENTS:
	 * DAT-632
	 * DAT-635
	 */
	static constexpr auto windows_1252_data_specifier =
		lexy::dsl::ascii::alpha_digit_underscore / LEXY_ASCII_ONE_OF("+:@%&'-.") /
		lexy::dsl::lit_b<0x8A> / lexy::dsl::lit_b<0x8C> / lexy::dsl::lit_b<0x8E> /
		lexy::dsl::lit_b<0x92> / lexy::dsl::lit_b<0x97> / lexy::dsl::lit_b<0x9A> / lexy::dsl::lit_b<0x9C> /
		detail::lexydsl::make_range<0x9E, 0x9F>() /
		detail::lexydsl::make_range<0xC0, 0xD6>() /
		detail::lexydsl::make_range<0xD8, 0xF6>() /
		detail::lexydsl::make_range<0xF8, 0xFF>();

	static constexpr auto windows_1251_data_specifier_additions =
		detail::lexydsl::make_range<0x80, 0x81>() / lexy::dsl::lit_b<0x83> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D> / lexy::dsl::lit_b<0x9F> /
		detail::lexydsl::make_range<0xA1, 0xA3>() / lexy::dsl::lit_b<0xA5> / lexy::dsl::lit_b<0xA8> / lexy::dsl::lit_b<0xAA> /
		lexy::dsl::lit_b<0xAF> /
		detail::lexydsl::make_range<0xB2, 0xB4>() / lexy::dsl::lit_b<0xB8> / lexy::dsl::lit_b<0xBA> /
		detail::lexydsl::make_range<0xBC, 0xBF>() /
		lexy::dsl::lit_b<0xD7> / lexy::dsl::lit_b<0xF7>;

	static constexpr auto data_specifier = windows_1252_data_specifier / windows_1251_data_specifier_additions;

	static constexpr auto data_char_class = LEXY_CHAR_CLASS("DataSpecifier", data_specifier);

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

	template<ParseOptions Options>
	struct Identifier {
		static constexpr auto rule = lexy::dsl::identifier(data_char_class);
		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](auto lexeme) {
				std::string str(lexeme.data(), lexeme.size());
				return ast::make_node_ptr<ast::IdentifierNode>(ast::NodeLocation { lexeme.begin(), lexeme.end() }, LEXY_MOV(str));
			});
	};

	/* REQUIREMENTS:
	 * DAT-633
	 * DAT-634
	 */
	template<ParseOptions Options>
	struct StringExpression {
		static constexpr auto rule = [] {
			if constexpr (Options.NoStringEscape) {
				auto c = ovdl::detail::lexydsl::make_range<0x20, 0xFF>() / lexy::dsl::lit_b<0x07> / lexy::dsl::lit_b<0x09> / lexy::dsl::lit_b<0x0A> / lexy::dsl::lit_b<0x0D>;
				return lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'"'>))(c);
			} else {
				// Arbitrary code points that aren't control characters.
				auto c = ovdl::detail::lexydsl::make_range<0x20, 0xFF>() - lexy::dsl::ascii::control;

				// Escape sequences start with a backlash.
				// They either map one of the symbols,
				// or a Unicode code point of the form uXXXX.
				auto escape = lexy::dsl::backslash_escape //
								  .symbol<escaped_symbols>();
				return lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'"'>))(c, escape);
			}
		}();

		static constexpr auto value =
			lexy::as_string<std::string> >>
			lexy::callback<ast::NodePtr>(
				[](const char* begin, auto&& str, const char* end) {
					return ast::make_node_ptr<ast::StringNode>(ast::NodeLocation::make_from(begin, end), LEXY_MOV(str), Options.NoStringEscape);
				});
	};

	/* REQUIREMENTS: DAT-638 */
	template<ParseOptions Options>
	struct ValueExpression {
		static constexpr auto rule = lexy::dsl::p<Identifier<Options>> | lexy::dsl::p<StringExpression<Options>>;
		static constexpr auto value = lexy::forward<ast::NodePtr>;
	};

	template<ParseOptions Options>
	struct SimpleAssignmentStatement {
		static constexpr auto rule =
			lexy::dsl::position(lexy::dsl::p<Identifier<Options>>) >>
			(lexy::dsl::equal_sign +
				(lexy::dsl::p<ValueExpression<Options>> | lexy::dsl::recurse_branch<StatementListBlock<Options>>));

		static constexpr auto value = lexy::callback<ast::NodePtr>(
			[](const char* pos, auto name, auto&& initalizer) {
				return ast::make_node_ptr<ast::AssignNode>(pos, LEXY_MOV(name), LEXY_MOV(initalizer));
			});
	};

	/* REQUIREMENTS: DAT-639 */
	template<ParseOptions Options>
	struct AssignmentStatement {
		static constexpr auto rule =
			lexy::dsl::position(lexy::dsl::p<Identifier<Options>>) >>
				(lexy::dsl::equal_sign >>
						(lexy::dsl::p<ValueExpression<Options>> | lexy::dsl::recurse_branch<StatementListBlock<Options>>) |
					lexy::dsl::else_ >> lexy::dsl::return_) |
			lexy::dsl::p<StringExpression<Options>> |
			lexy::dsl::recurse_branch<StatementListBlock<Options>>;

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

	/* REQUIREMENTS: DAT-640 */
	template<ParseOptions Options>
	struct StatementListBlock {
		static constexpr auto rule =
			lexy::dsl::position(lexy::dsl::curly_bracketed.open()) >>
			(lexy::dsl::opt(lexy::dsl::list(lexy::dsl::recurse_branch<AssignmentStatement<Options>>)) +
				lexy::dsl::opt(lexy::dsl::semicolon)) >>
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
				[](const char* begin, lexy::nullopt, lexy::nullopt, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end));
				},
				[](const char* begin, auto&& list, lexy::nullopt, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end), LEXY_MOV(list));
				},
				[](const char* begin, auto& list, const char* end) {
					return ast::make_node_ptr<ast::ListNode>(ast::NodeLocation::make_from(begin, end), list);
				});
	};

	template<ParseOptions Options>
	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<AssignmentStatement<Options>>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};
}
