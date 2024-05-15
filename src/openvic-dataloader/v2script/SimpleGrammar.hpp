#pragma once

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/identifier.hpp>
#include <lexy/dsl/symbol.hpp>

#include "ParseState.hpp"
#include "detail/dsl.hpp"

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
	template<typename T>
	constexpr auto construct = dsl::construct<ast::ParseState, T>;
	template<typename T, bool DisableEmpty = false, typename ListType = ast::AssignStatementList>
	constexpr auto construct_list = dsl::construct_list<ast::ParseState, T, ListType, DisableEmpty>;

	struct ParseOptions {
		/// @brief Makes string parsing avoid string escapes
		bool NoStringEscape;
	};

	static constexpr ParseOptions NoStringEscapeOption = ParseOptions { true };
	static constexpr ParseOptions StringEscapeOption = ParseOptions { false };

	/* REQUIREMENTS: DAT-630 */
	static constexpr auto whitespace_specifier = lexy::dsl::ascii::blank / lexy::dsl::ascii::newline;
	/* REQUIREMENTS: DAT-631 */
	static constexpr auto comment_specifier = LEXY_LIT("#") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	static constexpr auto ascii = lexy::dsl::ascii::alpha_digit_underscore / LEXY_ASCII_ONE_OF("+:@%&'-.");

	/* REQUIREMENTS:
	 * DAT-632
	 * DAT-635
	 */
	static constexpr auto windows_1252_data_specifier =
		ascii /
		lexy::dsl::lit_b<0x8A> / lexy::dsl::lit_b<0x8C> / lexy::dsl::lit_b<0x8E> /
		lexy::dsl::lit_b<0x92> / lexy::dsl::lit_b<0x97> / lexy::dsl::lit_b<0x9A> / lexy::dsl::lit_b<0x9C> /
		dsl::make_range<0x9E, 0x9F>() /
		dsl::make_range<0xC0, 0xD6>() /
		dsl::make_range<0xD8, 0xF6>() /
		dsl::make_range<0xF8, 0xFF>();

	static constexpr auto windows_1251_data_specifier_additions =
		dsl::make_range<0x80, 0x81>() / lexy::dsl::lit_b<0x83> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D> / lexy::dsl::lit_b<0x9F> /
		dsl::make_range<0xA1, 0xA3>() / lexy::dsl::lit_b<0xA5> / lexy::dsl::lit_b<0xA8> / lexy::dsl::lit_b<0xAA> /
		lexy::dsl::lit_b<0xAF> /
		dsl::make_range<0xB2, 0xB4>() / lexy::dsl::lit_b<0xB8> / lexy::dsl::lit_b<0xBA> /
		dsl::make_range<0xBC, 0xBF>() /
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

	static constexpr auto id = lexy::dsl::identifier(data_char_class);

	template<ParseOptions Options>
	struct SimpleGrammar {
		struct StatementListBlock;

		struct Identifier {
			static constexpr auto rule = lexy::dsl::identifier(data_char_class);
			static constexpr auto value = dsl::callback<ast::IdentifierValue*>(
				[](ast::ParseState& state, auto lexeme) {
					auto value = state.ast().intern(lexeme.data(), lexeme.size());
					return state.ast().create<ast::IdentifierValue>(ovdl::NodeLocation::make_from(lexeme.begin(), lexeme.end()), value);
				});
		};

		/* REQUIREMENTS:
		 * DAT-633
		 * DAT-634
		 */
		struct StringExpression {
			static constexpr auto rule = [] {
				if constexpr (Options.NoStringEscape) {
					auto c = dsl::make_range<0x20, 0xFF>() / lexy::dsl::lit_b<0x07> / lexy::dsl::lit_b<0x09> / lexy::dsl::lit_b<0x0A> / lexy::dsl::lit_b<0x0D>;
					return lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'"'>))(c);
				} else {
					// Arbitrary code points that aren't control characters.
					auto c = dsl::make_range<0x20, 0xFF>() - lexy::dsl::ascii::control;

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
				dsl::callback<ast::StringValue*>(
					[](ast::ParseState& state, const char* begin, auto&& str, const char* end) {
						auto value = state.ast().intern(str.data(), str.length());
						return state.ast().create<ast::StringValue>(ovdl::NodeLocation::make_from(begin, end), value);
					});
		};

		/* REQUIREMENTS: DAT-638 */
		struct ValueExpression {
			static constexpr auto rule = lexy::dsl::p<Identifier> | lexy::dsl::p<StringExpression>;
			static constexpr auto value = lexy::forward<ast::Value*>;
		};

		struct SimpleAssignmentStatement {
			static constexpr auto rule =
				dsl::p<Identifier> >>
				(lexy::dsl::equal_sign >>
					(lexy::dsl::p<ValueExpression> | lexy::dsl::recurse_branch<StatementListBlock>));

			static constexpr auto value = construct<ast::AssignStatement>;
		};

		/* REQUIREMENTS: DAT-639 */
		struct AssignmentStatement {
			static constexpr auto rule =
				dsl::p<Identifier> >>
					(lexy::dsl::equal_sign >>
							(lexy::dsl::p<ValueExpression> | lexy::dsl::recurse_branch<StatementListBlock>) |
						lexy::dsl::else_ >> lexy::dsl::return_) |
				dsl::p<StringExpression> |
				lexy::dsl::recurse_branch<StatementListBlock>;

			static constexpr auto value = dsl::callback<ast::Statement*>(
				[](ast::ParseState& state, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
					return state.ast().create<ast::AssignStatement>(pos, name, initializer);
				},
				[](ast::ParseState& state, const char* pos, ast::Value* left, lexy::nullopt = {}) {
					return state.ast().create<ast::ValueStatement>(pos, left);
				},
				[](ast::ParseState& state, ast::Value* left) {
					return state.ast().create<ast::ValueStatement>(state.ast().location_of(left), left);
				});
		};

		/* REQUIREMENTS: DAT-640 */
		struct StatementListBlock {
			static constexpr auto rule =
				dsl::curly_bracketed(
					(lexy::dsl::opt(lexy::dsl::list(lexy::dsl::recurse_branch<AssignmentStatement>)) +
						lexy::dsl::opt(lexy::dsl::semicolon)));

			static constexpr auto value =
				lexy::as_list<ast::StatementList> >>
				dsl::callback<ast::ListValue*>(
					[](ast::ParseState& state, const char* begin, auto&& list, const char* end) {
						if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
							return state.ast().create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
						} else {
							return state.ast().create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
						}
					},
					[](ast::ParseState& state, const char* begin, auto&& list, auto&& semicolon, const char* end) {
						if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
							return state.ast().create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
						} else {
							return state.ast().create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
						}
					});
		};
	};

	template<ParseOptions Options>
	using StringExpression = typename SimpleGrammar<Options>::StringExpression;

	template<ParseOptions Options>
	using Identifier = typename SimpleGrammar<Options>::Identifier;

	template<ParseOptions Options>
	using SAssignStatement = typename SimpleGrammar<Options>::SimpleAssignmentStatement;

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::ParseState, ast::IdentifierValue, Keyword>>
	using keyword_rule = dsl::keyword_rule<
		ast::ParseState,
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::ParseState, ast::IdentifierValue, Keyword>>
	using fkeyword_rule = dsl::fkeyword_rule<
		ast::ParseState,
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	template<ParseOptions Options>
	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position(
			lexy::dsl::terminator(lexy::dsl::eof)
				.opt_list(lexy::dsl::p<typename SimpleGrammar<Options>::AssignmentStatement>));

		static constexpr auto value = lexy::as_list<ast::StatementList> >> construct<ast::FileTree>;
	};
}
