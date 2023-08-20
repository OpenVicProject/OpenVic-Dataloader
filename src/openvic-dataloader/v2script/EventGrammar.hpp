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

// Event Grammar Definitions //
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
	static constexpr auto event_symbols = lexy::symbol_table<ast::EventNode::Type> //
											  .map<LEXY_SYMBOL("country_event")>(ast::EventNode::Type::Country)
											  .map<LEXY_SYMBOL("province_event")>(ast::EventNode::Type::Province);

	struct EventMeanTimeToHappenModifierStatement {
		static constexpr auto rule =
			modifier_keyword >>
			lexy::dsl::curly_bracketed.list(
				(factor_keyword >> lexy::dsl::p<Identifier>) |
				lexy::dsl::p<TriggerStatement>);
	};

	struct EventMeanTimeToHappenStatement {
		static constexpr auto months_keyword = LEXY_KEYWORD("months", lexy::dsl::inline_<Identifier>);

		static constexpr auto rule = lexy::dsl::list(
			(months_keyword >> lexy::dsl::p<Identifier>) |
			lexy::dsl::p<EventMeanTimeToHappenModifierStatement>);
	};

	struct EventOptionList {
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(name);

		static constexpr auto rule = [] {
			constexpr auto create_flags = name_flag.create();

			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(name, lexy::dsl::p<StringExpression> | lexy::dsl::p<Identifier>);

			return create_flags + lexy::dsl::list(name_statement | lexy::dsl::p<EffectStatement>);
		}();
	};

	struct EventStatement {
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(id);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(title);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(desc);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(picture);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(is_triggered_only);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(fire_only_once);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(immediate);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(mean_time_to_happen);
		OVDL_GRAMMAR_KEYWORD_DEFINE(trigger);
		OVDL_GRAMMAR_KEYWORD_DEFINE(option);

		static constexpr auto rule = [] {
			constexpr auto create_flags =
				id_flag.create() +
				title_flag.create() +
				desc_flag.create() +
				picture_flag.create() +
				is_triggered_only_flag.create() +
				fire_only_once_flag.create() +
				immediate_flag.create() +
				mean_time_to_happen_flag.create();

			constexpr auto string_value = lexy::dsl::p<StringExpression> | lexy::dsl::p<Identifier>;

			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(id, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(title, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(desc, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(picture, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(is_triggered_only, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(fire_only_once, string_value);
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(immediate, lexy::dsl::curly_bracketed.opt(lexy::dsl::p<EffectList>));
			OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT(mean_time_to_happen, lexy::dsl::curly_bracketed(lexy::dsl::p<EventMeanTimeToHappenStatement>));

			OVDL_GRAMMAR_KEYWORD_STATEMENT(trigger, lexy::dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>));
			OVDL_GRAMMAR_KEYWORD_STATEMENT(option, lexy::dsl::curly_bracketed(lexy::dsl::p<EventOptionList>));

			return lexy::dsl::symbol<event_symbols>(lexy::dsl::inline_<Identifier>) >>
				   (create_flags + lexy::dsl::equal_sign +
					   lexy::dsl::curly_bracketed.opt_list(
						   id_statement |
						   title_statement |
						   desc_statement |
						   picture_statement |
						   is_triggered_only_statement |
						   fire_only_once_statement |
						   immediate_statement |
						   mean_time_to_happen_statement |
						   trigger_statement |
						   option_statement |
						   lexy::dsl::p<SimpleAssignmentStatement>));
		}();

		static constexpr auto value = lexy::callback<ast::NodePtr>([](auto name, lexy::nullopt = {}) { return LEXY_MOV(name); }, [](auto name, auto&& initalizer) { return make_node_ptr<ast::AssignNode>(LEXY_MOV(name), LEXY_MOV(initalizer)); });
	};

	struct EventFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::terminator(lexy::dsl::eof).list(lexy::dsl::p<EventStatement> | lexy::dsl::p<SimpleAssignmentStatement>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};

#undef OVDL_GRAMMAR_KEYWORD_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_STATEMENT
#undef OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT
}