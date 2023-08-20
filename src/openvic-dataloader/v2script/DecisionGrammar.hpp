#pragma once

#include <memory>
#include <string>
#include <vector>

#include "AiBehaviorGrammar.hpp"
#include "EffectGrammar.hpp"
#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"
#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

// Decision Grammar Definitions //
namespace ovdl::v2script::grammar {
	//////////////////
	// Macros
	//////////////////
// Produces <KW_NAME>_keyword
#define OVDL_GRAMMAR_KEYWORD_DEFINE(KW_NAME) \
	static constexpr auto KW_NAME##_keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier>)

// Produces <KW_NAME>_keyword and <KW_NAME>_flag and <KW_NAME>_too_many_error
#define OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(KW_NAME)                                                     \
	static constexpr auto KW_NAME##_keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier>); \
	static constexpr auto KW_NAME##_flag = lexy::dsl::context_flag<struct KW_NAME##_context>;         \
	struct KW_NAME##_too_many_error {                                                                 \
		static constexpr auto name = "expected left side " #KW_NAME " to be found once";              \
	}

// Produces <KW_NAME>_statement
#define OVDL_GRAMMAR_KEYWORD_STATEMENT(KW_NAME, ...) \
	constexpr auto KW_NAME##_statement = KW_NAME##_keyword >> (lexy::dsl::equal_sign + (__VA_ARGS__))

// Produces <KW_NAME>_statement
#define OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(KW_NAME, ...) \
	constexpr auto KW_NAME##_statement = KW_NAME##_keyword >> ((lexy::dsl::must(KW_NAME##_flag.is_reset()).error<KW_NAME##_too_many_error> + KW_NAME##_flag.set()) + lexy::dsl::equal_sign + (__VA_ARGS__))
	//////////////////
	// Macros
	//////////////////

	struct DecisionStatement {
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(potential);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(allow);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(effect);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(ai_will_do);

		static constexpr auto rule = [] {
			constexpr auto create_flags =
				potential_flag.create() +
				allow_flag.create() +
				effect_flag.create() +
				ai_will_do_flag.create();
			constexpr auto check_flag = [](auto flag) { return flag.is_reset() + flag.set(); };

			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(potential, lexy::dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>));
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(allow, lexy::dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>));
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(effect, lexy::dsl::curly_bracketed(lexy::dsl::p<EffectList>));
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(ai_will_do, lexy::dsl::curly_bracketed(lexy::dsl::p<AiBehaviorList>));

			return lexy::dsl::p<Identifier> >>
				   (create_flags + lexy::dsl::equal_sign +
					   lexy::dsl::curly_bracketed.list(
						   potential_statement |
						   allow_statement |
						   effect_statement |
						   ai_will_do_statement |
						   lexy::dsl::p<SimpleAssignmentStatement>));
		}();

		static constexpr auto value = lexy::callback<ast::NodePtr>([](auto name, lexy::nullopt = {}) { return LEXY_MOV(name); }, [](auto name, auto&& initalizer) { return make_node_ptr<ast::AssignNode>(LEXY_MOV(name), LEXY_MOV(initalizer)); });
	};

	struct DecisionFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::terminator(lexy::dsl::eof).list(																									  //
			(LEXY_KEYWORD("political_decisions", lexy::dsl::inline_<Identifier>) >> (lexy::dsl::equal_sign + lexy::dsl::curly_bracketed.opt_list(lexy::dsl::p<DecisionStatement>))) | //
			lexy::dsl::p<SimpleAssignmentStatement>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};

#undef OVDL_GRAMMAR_KEYWORD_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_STATEMENT
#undef OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT
}