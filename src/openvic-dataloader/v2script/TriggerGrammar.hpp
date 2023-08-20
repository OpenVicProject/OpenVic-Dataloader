#pragma once

#include "SimpleGrammar.hpp"
#include <lexy/dsl.hpp>

namespace ovdl::v2script::grammar {
	struct TriggerStatement {
		static constexpr auto rule = lexy::dsl::p<SimpleAssignmentStatement>;
	};

	struct TriggerList {
		static constexpr auto rule = lexy::dsl::list(lexy::dsl::p<TriggerStatement>);
	};
}