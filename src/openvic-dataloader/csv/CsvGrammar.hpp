#pragma once

#include <initializer_list>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include "detail/LexyLitRange.hpp"

// Grammar Definitions //
namespace ovdl::csv::grammar::windows1252 {
	constexpr auto character = detail::lexydsl::make_range<0x01, 0xFF>();
	constexpr auto control =
		lexy::dsl::ascii::control /
		lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;

#include "Grammar.inc"
}

namespace ovdl::csv::grammar::utf8 {
	constexpr auto character = lexy::dsl::unicode::character;
	constexpr auto control = lexy::dsl::unicode::control;

#include "Grammar.inc"
}