#pragma once

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/code_point.hpp>
#include <lexy/dsl/delimited.hpp>
#include <lexy/dsl/integer.hpp>
#include <lexy/dsl/literal.hpp>
#include <lexy/dsl/position.hpp>
#include <lexy/dsl/unicode.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/base.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/lexeme.hpp>

#include "detail/InternalConcepts.hpp"
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
	constexpr auto construct = dsl::construct<T>;
	template<typename T, bool DisableEmpty = false, typename ListType = ast::AssignStatementList>
	constexpr auto construct_list = dsl::construct_list<T, ListType, DisableEmpty>;

	struct ConvertErrorHandler {
		static constexpr void on_invalid_character(detail::IsStateType auto& state, auto reader) {
			state.logger().warning("invalid character value '{}' found.", static_cast<int>(reader.peek())) //
				.primary(BasicNodeLocation { reader.position() }, "here")
				.finish();
		}
	};

	/* REQUIREMENTS: DAT-630 */
	static constexpr auto whitespace_specifier = lexy::dsl::ascii::blank / lexy::dsl::ascii::newline;
	/* REQUIREMENTS: DAT-631 */
	static constexpr auto comment_specifier = LEXY_LIT("#") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	/* REQUIREMENTS:
	 * DAT-632
	 * DAT-635
	 */
	static constexpr auto utf_data_specifier =
		lexy::dsl::unicode::xid_continue / LEXY_ASCII_ONE_OF("+:@%&'-.\\/") /
		lexy::dsl::code_point.range<U'À', U'Ö'>() /
		lexy::dsl::code_point.range<U'Ø', U'ö'>() /
		lexy::dsl::code_point.range<U'ø', U'ÿ'>() /
		lexy::dsl::code_point.set<U'Š', U'Œ', U'Ž', U'’', U'—', U'š', U'œ', U'ž', U'Ÿ'>() /
		lexy::dsl::code_point.set<U'Ђ', U'Ѓ', U'ѓ', U'Ќ', U'Џ', U'ђ', U'ќ', U'џ'>() /
		lexy::dsl::code_point.set<U'Ў', U'ў', U'Ј', U'Ґ', U'Ё', U'Є', U'Ї', U'І'>() /
		lexy::dsl::code_point.set<U'і', U'ґ', U'ё', U'є', U'ј', U'Ѕ', U'ѕ', U'ї'>() /
		lexy::dsl::code_point.set<U'Ч', U'ч'>();

	static constexpr auto utf_char_class = LEXY_CHAR_CLASS("DataSpecifier", utf_data_specifier);

	static constexpr auto id = lexy::dsl::identifier(utf_char_class);

	struct StatementListBlock;

	struct Identifier : lexy::token_production {
		static constexpr auto rule = lexy::dsl::identifier(utf_char_class);

		static constexpr auto value = dsl::callback<ast::IdentifierValue*>(
			[](detail::IsParseState auto& state, auto lexeme) {
				auto value = state.ast().intern(lexeme);
				return state.ast().template create<ast::IdentifierValue>(ovdl::NodeLocation::make_from(lexeme.begin(), lexeme.end()), value);
			});
	};

	/* REQUIREMENTS:
	 * DAT-633
	 * DAT-634
	 */
	struct StringExpression : lexy::token_production {
		static constexpr auto rule = lexy::dsl::quoted(lexy::dsl::unicode::character);
		static constexpr auto value =
			dsl::as_string_view<> >>
			dsl::callback<ast::StringValue*>(
				[](detail::IsParseState auto& state, std::string_view sv) {
					auto value = state.ast().intern(sv);
					return state.ast().template create<ast::StringValue>(ovdl::NodeLocation::make_from(sv.data(), sv.data() + sv.size()), value);
				});
	};

	/* REQUIREMENTS: DAT-638 */
	struct ValueExpression {
		static constexpr auto rule = lexy::dsl::p<Identifier> | lexy::dsl::p<StringExpression>;
		static constexpr auto value = lexy::forward<ast::Value*>;
	};

	struct SimpleAssignmentStatement {
		static constexpr auto rule = [] {
			auto right_brace = lexy::dsl::lit_c<'}'>;

			auto value_expression = lexy::dsl::p<ValueExpression>;
			auto statement_list_expression = lexy::dsl::recurse_branch<StatementListBlock>;

			auto rhs_recover = lexy::dsl::recover(value_expression, statement_list_expression).limit(right_brace);
			auto rhs_try = lexy::dsl::try_(value_expression | statement_list_expression, rhs_recover);

			auto identifier =
				dsl::p<Identifier> >>
				(lexy::dsl::equal_sign >> rhs_try);

			auto recover = lexy::dsl::recover(identifier).limit(right_brace);
			return lexy::dsl::try_(identifier, recover);
		}();

		static constexpr auto value = construct<ast::AssignStatement>;
	};

	/* REQUIREMENTS: DAT-639 */
	struct AssignmentStatement {
		static constexpr auto rule = [] {
			auto right_brace = lexy::dsl::lit_c<'}'>;

			auto value_expression = lexy::dsl::p<ValueExpression>;
			auto statement_list_expression = lexy::dsl::recurse_branch<StatementListBlock>;

			auto rhs_recover = lexy::dsl::recover(value_expression, statement_list_expression).limit(right_brace);
			auto rhs_try = lexy::dsl::try_(value_expression | statement_list_expression, rhs_recover);

			auto identifier =
				dsl::p<Identifier> >>
				(lexy::dsl::equal_sign >>
						rhs_try |
					lexy::dsl::else_ >> lexy::dsl::return_);

			auto string_expression = dsl::p<StringExpression>;
			auto statement_list = lexy::dsl::recurse_branch<StatementListBlock>;

			return identifier | string_expression | statement_list;
		}();

		static constexpr auto value = dsl::callback<ast::Statement*>(
			[](detail::IsParseState auto& state, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) -> ast::AssignStatement* {
				return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
			},
			[](detail::IsParseState auto& state, bool&, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
				return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
			},
			[](detail::IsParseState auto& state, bool&, bool&, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
				return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
			},
			[](detail::IsParseState auto& state, bool&, bool&, const char* pos, ast::Value* name) {
				return state.ast().template create<ast::ValueStatement>(pos, name);
			},
			[](detail::IsParseState auto& state, const char* pos, ast::Value* left, lexy::nullopt = {}) {
				return state.ast().template create<ast::ValueStatement>(pos, left);
			},
			[](detail::IsParseState auto& state, bool&, const char* pos, ast::Value* left, lexy::nullopt = {}) {
				return state.ast().template create<ast::ValueStatement>(pos, left);
			},
			[](detail::IsParseState auto& state, ast::Value* left) -> ast::ValueStatement* {
				if (left == nullptr) { // May no longer be necessary
					return nullptr;
				}
				return state.ast().template create<ast::ValueStatement>(state.ast().location_of(left), left);
			},
			[](detail::IsParseState auto& state, bool&, ast::Value* left) -> ast::ValueStatement* {
				if (left == nullptr) { // May no longer be necessary
					return nullptr;
				}
				return state.ast().template create<ast::ValueStatement>(state.ast().location_of(left), left);
			});
	};

	/* REQUIREMENTS: DAT-640 */
	struct StatementListBlock {
		static constexpr auto rule = [] {
			auto right_brace = lexy::dsl::lit_c<'}'>;

			auto assign_statement = lexy::dsl::recurse_branch<AssignmentStatement>;

			auto assign_try = lexy::dsl::try_(assign_statement, lexy::dsl::nullopt);
			auto assign_opt = lexy::dsl::opt(lexy::dsl::list(assign_try));

			auto curly_bracket = dsl::curly_bracketed(assign_opt + lexy::dsl::opt(lexy::dsl::semicolon));

			return curly_bracket;
		}();

		static constexpr auto value =
			lexy::as_list<ast::StatementList> >>
			dsl::callback<ast::ListValue*>(
				[](detail::IsParseState auto& state, const char* begin, auto&& list, const char* end) {
					if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
						return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
					} else {
						return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
					}
				},
				[](detail::IsParseState auto& state, const char* begin, auto&& list, auto&& semicolon, const char* end) {
					if constexpr (std::is_same_v<std::decay_t<decltype(list)>, lexy::nullopt>) {
						return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end));
					} else {
						return state.ast().template create<ast::ListValue>(ovdl::NodeLocation::make_from(begin, end), LEXY_MOV(list));
					}
				});
	};

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::IdentifierValue, Keyword>>
	using keyword_rule = dsl::keyword_rule<
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	template<ovdl::detail::string_literal Keyword, auto Production, auto Value = dsl::default_kw_value<ast::IdentifierValue, Keyword>>
	using fkeyword_rule = dsl::fkeyword_rule<
		id,
		ast::AssignStatement,
		Keyword, Production, Value>;

	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position(
			lexy::dsl::terminator(lexy::dsl::eof)
				.opt_list(lexy::dsl::p<AssignmentStatement>));

		static constexpr auto value = lexy::as_list<ast::StatementList> >> construct<ast::FileTree>;
	};
}
