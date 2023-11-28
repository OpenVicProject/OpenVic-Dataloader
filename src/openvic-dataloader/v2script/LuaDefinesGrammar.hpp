#pragma once

#include <lexy/_detail/config.hpp>
#include <lexy/dsl.hpp>

#include "openvic-dataloader/v2script/AbstractSyntaxTree.hpp"

#include "SimpleGrammar.hpp"
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

	struct ParseOptions {
	};

	template<ParseOptions Options>
	struct StatementListBlock;

	static constexpr auto comment_specifier = LEXY_LIT("--") >> lexy::dsl::until(lexy::dsl::newline).or_eof();

	template<ParseOptions Options>
	struct Identifier {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::alpha_underscore, lexy::dsl::ascii::alpha_digit_underscore);
		static constexpr auto value = callback<ast::IdentifierValue*>(
			[](ast::ParseState& state, auto lexeme) {
				auto value = state.ast().intern(lexeme.data(), lexeme.size());
				return state.ast().create<ast::IdentifierValue>(lexeme.begin(), lexeme.end(), value);
			});
	};

	template<ParseOptions Options>
	struct Value {
		static constexpr auto rule = lexy::dsl::identifier(lexy::dsl::ascii::digit / lexy::dsl::lit_c<'.'> / lexy::dsl::lit_c<'-'>);
		static constexpr auto value = callback<ast::IdentifierValue*>(
			[](ast::ParseState& state, auto lexeme) {
				auto value = state.ast().intern(lexeme.data(), lexeme.size());
				return state.ast().create<ast::IdentifierValue>(lexeme.begin(), lexeme.end(), value);
			});
	};

	template<ParseOptions Options>
	struct String {
		static constexpr auto rule = [] {
			// Arbitrary code points that aren't control characters.
			auto c = dsl::make_range<0x20, 0xFF>() - lexy::dsl::ascii::control;

			return lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'"'>))(c) | lexy::dsl::delimited(lexy::dsl::position(lexy::dsl::lit_b<'\''>))(c);
		}();

		static constexpr auto value =
			lexy::as_string<std::string> >>
			callback<ast::StringValue*>(
				[](ast::ParseState& state, const char* begin, const std::string& str, const char* end) {
					auto value = state.ast().intern(str.data(), str.length());
					return state.ast().create<ast::StringValue>(begin, end, value);
				});
	};

	template<ParseOptions Options>
	struct Expression {
		static constexpr auto rule = lexy::dsl::p<Value<Options>> | lexy::dsl::p<String<Options>>;
		static constexpr auto value = lexy::forward<ast::Value*>;
	};

	template<ParseOptions Options>
	struct AssignmentStatement {
		static constexpr auto rule =
			dsl::p<Identifier<Options>> >>
			lexy::dsl::equal_sign >>
			(lexy::dsl::p<Expression<Options>> | lexy::dsl::recurse_branch<StatementListBlock<Options>>);

		static constexpr auto value = callback<ast::AssignStatement*>(
			[](ast::ParseState& state, const char* pos, ast::IdentifierValue* name, ast::Value* initializer) {
				return state.ast().create<ast::AssignStatement>(pos, name, initializer);
			});
	};

	template<ParseOptions Options>
	struct StatementListBlock {
		static constexpr auto rule =
			dsl::curly_bracketed(
				lexy::dsl::opt(
					lexy::dsl::list(
						lexy::dsl::recurse_branch<AssignmentStatement<Options>>,
						lexy::dsl::trailing_sep(lexy::dsl::lit_c<','>))));

		static constexpr auto value =
			lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue>;
	};

	template<ParseOptions Options = ParseOptions {}>
	struct File {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = ovdl::v2script::grammar::whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).opt_list(lexy::dsl::p<AssignmentStatement<Options>>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct<ast::FileTree>;
	};
}