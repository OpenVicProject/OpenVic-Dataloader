#pragma once

#include <string>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "AiBehaviorGrammar.hpp"
#include "EffectGrammar.hpp"
#include "ModifierGrammar.hpp"
#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"

// Event Grammar Definitions //
namespace ovdl::v2script::grammar {
	//////////////////
	// Macros
	//////////////////
// Produces <KW_NAME>_rule and <KW_NAME>_p
#define OVDL_GRAMMAR_KEYWORD_DEFINE(KW_NAME)                                                                        \
	struct KW_NAME##_rule {                                                                                         \
		static constexpr auto keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier<StringEscapeOption>>); \
		static constexpr auto rule = keyword >> lexy::dsl::equal_sign;                                              \
		static constexpr auto value = lexy::noop;                                                                   \
	};                                                                                                              \
	static constexpr auto KW_NAME##_p = lexy::dsl::p<KW_NAME##_rule>

// Produces <KW_NAME>_rule and <KW_NAME>_p and <KW_NAME>_rule::flag and <KW_NAME>_rule::too_many_error
#define OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(KW_NAME)                                                                   \
	struct KW_NAME##_rule {                                                                                         \
		static constexpr auto keyword = LEXY_KEYWORD(#KW_NAME, lexy::dsl::inline_<Identifier<StringEscapeOption>>); \
		static constexpr auto rule = keyword >> lexy::dsl::equal_sign;                                              \
		static constexpr auto value = lexy::noop;                                                                   \
		static constexpr auto flag = lexy::dsl::context_flag<struct KW_NAME##_context>;                             \
		struct too_many_error {                                                                                     \
			static constexpr auto name = "expected left side " #KW_NAME " to be found once";                        \
		};                                                                                                          \
	};                                                                                                              \
	static constexpr auto KW_NAME##_p = lexy::dsl::p<KW_NAME##_rule> >> (lexy::dsl::must(KW_NAME##_rule::flag.is_reset()).error<KW_NAME##_rule::too_many_error> + KW_NAME##_rule::flag.set())
	//////////////////
	// Macros
	//////////////////
	static constexpr auto event_symbols = lexy::symbol_table<ast::EventNode::Type> //
											  .map<LEXY_SYMBOL("country_event")>(ast::EventNode::Type::Country)
											  .map<LEXY_SYMBOL("province_event")>(ast::EventNode::Type::Province);

	struct EventMtthStatement {
		OVDL_GRAMMAR_KEYWORD_DEFINE(months);

		struct MonthValue {
			static constexpr auto rule = lexy::dsl::inline_<Identifier<StringEscapeOption>>;
			static constexpr auto value = lexy::as_string<std::string> | lexy::new_<ast::MonthNode, ast::NodePtr>;
		};

		static constexpr auto rule = lexy::dsl::list(
			(months_p >> lexy::dsl::p<MonthValue>) |
			lexy::dsl::p<ModifierStatement>);

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::MtthNode>(LEXY_MOV(list));
				});
	};

	template<auto Production, typename AstNode>
	struct _StringStatement {
		static constexpr auto rule = Production >> (lexy::dsl::p<StringExpression<StringEscapeOption>> | lexy::dsl::p<Identifier<StringEscapeOption>>);
		static constexpr auto value =
			lexy::callback<ast::NodePtr>(
				[](auto&& value) {
					auto result = ast::make_node_ptr<AstNode>(std::move(static_cast<ast::AbstractStringNode*>(value)->_name));
					delete value;
					return result;
				});
	};
	template<auto Production, typename AstNode>
	static constexpr auto StringStatement = lexy::dsl::p<_StringStatement<Production, AstNode>>;

	struct EventOptionList {
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(name);
		OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE(ai_chance);

		static constexpr auto rule = [] {
			constexpr auto create_flags = name_rule::flag.create() + ai_chance_rule::flag.create();

			constexpr auto name_statement = StringStatement<name_p, ast::NameNode>;
			constexpr auto ai_chance_statement = ai_chance_p >> lexy::dsl::curly_bracketed(lexy::dsl::p<AiBehaviorList>);

			return create_flags + lexy::dsl::list(name_statement | ai_chance_statement | lexy::dsl::p<EffectList>);
		}();

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto&& list) {
					return ast::make_node_ptr<ast::EventOptionNode>(LEXY_MOV(list));
				});
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
			constexpr auto symbol_value = lexy::dsl::symbol<event_symbols>(lexy::dsl::inline_<Identifier<StringEscapeOption>>);

			constexpr auto create_flags =
				id_rule::flag.create() +
				title_rule::flag.create() +
				desc_rule::flag.create() +
				picture_rule::flag.create() +
				is_triggered_only_rule::flag.create() +
				fire_only_once_rule::flag.create() +
				immediate_rule::flag.create() +
				mean_time_to_happen_rule::flag.create();

			constexpr auto id_statement = StringStatement<id_p, ast::IdNode>;
			constexpr auto title_statement = StringStatement<title_p, ast::TitleNode>;
			constexpr auto desc_statement = StringStatement<desc_p, ast::DescNode>;
			constexpr auto picture_statement = StringStatement<picture_p, ast::PictureNode>;
			constexpr auto is_triggered_only_statement = StringStatement<is_triggered_only_p, ast::IsTriggeredNode>;
			constexpr auto fire_only_once_statement = StringStatement<fire_only_once_p, ast::FireOnlyNode>;
			constexpr auto immediate_statement = immediate_p >> lexy::dsl::p<EffectBlock>;
			constexpr auto mean_time_to_happen_statement = mean_time_to_happen_p >> lexy::dsl::curly_bracketed(lexy::dsl::p<EventMtthStatement>);

			constexpr auto trigger_statement = trigger_p >> lexy::dsl::curly_bracketed.opt(lexy::dsl::p<TriggerList>);
			constexpr auto option_statement = option_p >> lexy::dsl::curly_bracketed(lexy::dsl::p<EventOptionList>);

			return symbol_value >>
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
						   lexy::dsl::p<SimpleAssignmentStatement<StringEscapeOption>>));
		}();

		static constexpr auto value =
			lexy::as_list<std::vector<ast::NodePtr>> >>
			lexy::callback<ast::NodePtr>(
				[](auto& type, auto&& list) {
					return ast::make_node_ptr<ast::EventNode>(type, LEXY_MOV(list));
				},
				[](auto& type, lexy::nullopt = {}) {
					return ast::make_node_ptr<ast::EventNode>(type);
				});
	};

	struct EventFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::terminator(lexy::dsl::eof).list(lexy::dsl::p<EventStatement> | lexy::dsl::p<SimpleAssignmentStatement<StringEscapeOption>>);

		static constexpr auto value = lexy::as_list<std::vector<ast::NodePtr>> >> lexy::new_<ast::FileNode, ast::NodePtr>;
	};

#undef OVDL_GRAMMAR_KEYWORD_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_FLAG_DEFINE
#undef OVDL_GRAMMAR_KEYWORD_STATEMENT
#undef OVDL_GRAMMAR_KEYWORD_FLAG_STATEMENT
}