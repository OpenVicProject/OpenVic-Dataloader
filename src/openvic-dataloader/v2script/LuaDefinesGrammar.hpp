#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/_detail/config.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/delimited.hpp>
#include <lexy/dsl/recover.hpp>
#include <lexy/dsl/unicode.hpp>

#include "SimpleGrammar.hpp"
#include "detail/InternalConcepts.hpp"
#include "detail/dsl.hpp"

namespace ovdl::v2script::lua::grammar {
	template<typename ReturnType, typename... Callback>
	constexpr auto callback(Callback... cb) {
		return dsl::callback<ReturnType>(cb...);
	}

	template<typename T>
	constexpr auto construct = v2script::grammar::construct<T>;

	template<typename T>
	constexpr auto construct_list = v2script::grammar::construct_list<T>;

	struct StatementListBlock;

	static constexpr auto comment_specifier = LEXY_LIT("--") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	struct Identifier {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::alpha_underscore, lexy::dsl::ascii::alpha_digit_underscore);
		static constexpr auto value =
			callback<ast::IdentifierValue*>(
				[](detail::IsParseState auto& state, auto lexeme) {
					auto value = state.ast().intern(lexeme.data(), lexeme.size());
					return state.ast().template create<ast::IdentifierValue>(lexeme.begin(), lexeme.end(), value);
				});
	};

	struct Value {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::digit / lexy::dsl::lit_c<'.'> / lexy::dsl::lit_c<'-'>);
		static constexpr auto value =
			callback<ast::IdentifierValue*>(
				[](detail::IsParseState auto& state, auto lexeme) {
					auto value = state.ast().intern(lexeme.data(), lexeme.size());
					return state.ast().template create<ast::IdentifierValue>(lexeme.begin(), lexeme.end(), value);
				});
	};

	struct String : lexy::token_production {
		static constexpr auto rule = lexy::dsl::quoted(-lexy::dsl::unicode::control) | lexy::dsl::single_quoted(-lexy::dsl::unicode::control);

		static constexpr auto value =
			dsl::as_string_view<> >>
			dsl::callback<ast::StringValue*>(
				[](detail::IsParseState auto& state, std::string_view sv) {
					auto value = state.ast().intern(sv);
					return state.ast().template create<ast::StringValue>(ovdl::NodeLocation::make_from(sv.data(), sv.data() + sv.size()), value);
				});
	};

	struct Expression {
		static constexpr auto rule = lexy::dsl::p<Value> | lexy::dsl::p<String>;
		static constexpr auto value = lexy::forward<ast::Value*>;
	};

	struct AssignmentStatement {
		static constexpr auto rule = [] {
			auto right_brace = lexy::dsl::lit_c<'}'>;

			auto expression = lexy::dsl::p<Expression>;
			auto statement_list = lexy::dsl::recurse_branch<StatementListBlock>;

			auto rhs_recover = lexy::dsl::recover(expression, statement_list).limit(right_brace);
			auto rhs_try = lexy::dsl::try_(expression | statement_list, rhs_recover);

			auto identifier = dsl::p<Identifier> >> lexy::dsl::equal_sign + rhs_try;

			auto recover = lexy::dsl::recover(identifier).limit(right_brace);
			return lexy::dsl::try_(identifier, recover);
		}();

		static constexpr auto value = callback<ast::AssignStatement*>(
			[](detail::IsParseState auto& state, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) -> ast::AssignStatement* {
				if (initializer == nullptr) {
					return nullptr;
				}
				return state.ast().template create<ast::AssignStatement>(pos, name, initializer);
			},
			[](detail::IsParseState auto& state, ast::Value*) {
				return nullptr;
			},
			[](detail::IsParseState auto& state) {
				return nullptr;
			});
	};

	struct StatementListBlock {
		static constexpr auto rule = [] {
			auto right_brace = lexy::dsl::lit_c<'}'>;
			auto comma = lexy::dsl::lit_c<','>;

			auto assign_statement = lexy::dsl::recurse_branch<AssignmentStatement>;
			auto assign_try = lexy::dsl::try_(assign_statement);

			auto curly_bracket = dsl::curly_bracketed.opt_list(
				assign_try,
				lexy::dsl::trailing_sep(comma));

			return lexy::dsl::try_(curly_bracket, lexy::dsl::find(right_brace));
		}();

		static constexpr auto value =
			lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue>;
	};

	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = ovdl::v2script::grammar::whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<AssignmentStatement>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct<ast::FileTree>;
	};
}