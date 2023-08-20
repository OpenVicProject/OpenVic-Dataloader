#pragma once

#include "SimpleGrammar.hpp"
#include "TriggerGrammar.hpp"
#include <lexy/dsl.hpp>

namespace ovdl::v2script::grammar {
	constexpr auto modifier_keyword = LEXY_KEYWORD("modifier", lexy::dsl::inline_<Identifier>);
	constexpr auto factor_keyword = LEXY_KEYWORD("factor", lexy::dsl::inline_<Identifier>);

	struct AiModifierStatement {
		static constexpr auto rule =
			modifier_keyword >>
			lexy::dsl::curly_bracketed.list(
				(factor_keyword >> lexy::dsl::p<Identifier>) |
				lexy::dsl::p<TriggerStatement>);
	};

	struct AiBehaviorList {
		static constexpr auto rule = lexy::dsl::list((factor_keyword >> lexy::dsl::p<Identifier>) | lexy::dsl::p<AiModifierStatement>);
	};
}