#pragma once

#include "SimpleGrammar.hpp"
#include <lexy/dsl.hpp>

namespace ovdl::v2script::grammar {
	struct EffectStatement {
		static constexpr auto rule = lexy::dsl::p<SimpleAssignmentStatement>;
	};

	struct EffectList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<EffectStatement>);
	};
}