#pragma once

#include <cctype>
#include <cstdlib>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>
#include <lexy/grammar.hpp>

#include "openvic-dataloader/NodeLocation.hpp"

#include "SimpleGrammar.hpp"
#include "detail/InternalConcepts.hpp"
#include "detail/dsl.hpp"
#include "v2script/AiBehaviorGrammar.hpp"
#include "v2script/EffectGrammar.hpp"
#include "v2script/ModifierGrammar.hpp"

// Event Grammar Definitions //
namespace ovdl::v2script::grammar {
	static constexpr auto event_symbols = lexy::symbol_table<bool> //
											  .map<LEXY_SYMBOL("country_event")>(false)
											  .map<LEXY_SYMBOL("province_event")>(true);

	struct EventMtthStatement {
		struct MonthValue {
			static constexpr auto rule = lexy::dsl::p<Identifier<StringEscapeOption>>;
			static constexpr auto value = dsl::callback<ast::IdentifierValue*>(
				[](detail::IsParseState auto& state, ast::IdentifierValue* value) {
					bool is_number = true;
					for (auto* current = value->value(state.ast().symbol_interner()); *current; current++) {
						is_number = is_number && std::isdigit(*current);
						if (!is_number) break;
					}
					if (!is_number) {
						state.logger().warning("month is not an integer") //
							.primary(state.ast().location_of(value), "here")
							.finish();
					}
					return value;
				});
		};

		using months = keyword_rule<"months", lexy::dsl::p<MonthValue>>;

		static constexpr auto rule = dsl::curly_bracketed(lexy::dsl::p<months> | lexy::dsl::p<ModifierStatement>);

		static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue, true>;
	};

	static constexpr auto str_or_id = lexy::dsl::p<SimpleGrammar<StringEscapeOption>::ValueExpression>;

	struct EventOptionList {
		using name = fkeyword_rule<"name", str_or_id>;
		using ai_chance = fkeyword_rule<"ai_chance", lexy::dsl::p<AiBehaviorBlock>>;

		static constexpr auto rule = [] {
			using helper = dsl::rule_helper<name, ai_chance>;

			return dsl::curly_bracketed(helper::flags + lexy::dsl::list(helper::p | lexy::dsl::p<EffectStatement>));
		}();

		static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue, true>;
	};

	struct EventStatement {
		using id = fkeyword_rule<"id", str_or_id>;
		using title = fkeyword_rule<"title", str_or_id>;
		using desc = fkeyword_rule<"desc", str_or_id>;
		using picture = fkeyword_rule<"picture", str_or_id>;
		using is_triggered_only = fkeyword_rule<"is_triggered_only", str_or_id>;
		using fire_only_once = fkeyword_rule<"fire_only_once", str_or_id>;
		using immediate = fkeyword_rule<"immediate", lexy::dsl::p<EffectBlock>>;
		using mean_time_to_happen = fkeyword_rule<"mean_time_to_happen", lexy::dsl::p<EventMtthStatement>>;
		using trigger = keyword_rule<"trigger", lexy::dsl::p<TriggerBlock>>;
		using option = keyword_rule<"option", lexy::dsl::p<EventOptionList>>;

		struct EventList {
			static constexpr auto rule = [] {
				using helper = dsl::rule_helper<id, title, desc, picture, is_triggered_only, fire_only_once, immediate, mean_time_to_happen>;

				return helper::flags +
					   dsl::curly_bracketed.opt_list(
						   helper::p | lexy::dsl::p<trigger> | lexy::dsl::p<option> |
						   lexy::dsl::p<SAssignStatement<StringEscapeOption>>);
			}();

			static constexpr auto value = lexy::as_list<ast::AssignStatementList> >> construct_list<ast::ListValue, true>;
		};

		static constexpr auto rule = dsl::p<Identifier<StringEscapeOption>> >> lexy::dsl::equal_sign >> lexy::dsl::p<EventList>;

		static constexpr auto value =
			dsl::callback<ast::EventStatement*>(
				[](detail::IsParseState auto& state, NodeLocation loc, ast::IdentifierValue* name, ast::ListValue* list) {
					static auto country_decl = state.ast().intern_cstr("country_event");
					static auto province_decl = state.ast().intern_cstr("province_event");

					if (name->value(state.ast().symbol_interner()) != country_decl || name->value(state.ast().symbol_interner()) != province_decl) {
						state.logger().warning("event declarator \"{}\" is not {} or {}", name->value(state.ast().symbol_interner()), country_decl, province_decl) //
							.primary(loc, "here")
							.finish();
					}

					return state.ast().template create<ast::EventStatement>(loc, name->value(state.ast().symbol_interner()) == province_decl, list);
				});
	};

	struct EventFile {
		// Allow arbitrary spaces between individual tokens.
		static constexpr auto whitespace = whitespace_specifier | comment_specifier;

		static constexpr auto rule = lexy::dsl::position + lexy::dsl::terminator(lexy::dsl::eof).list(lexy::dsl::p<EventStatement> | lexy::dsl::p<SAssignStatement<StringEscapeOption>>);

		static constexpr auto value = lexy::as_list<ast::StatementList> >> construct<ast::FileTree>;
	};
}