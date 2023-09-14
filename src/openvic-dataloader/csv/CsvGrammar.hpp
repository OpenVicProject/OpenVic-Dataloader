#pragma once

#include <initializer_list>
#include <memory>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/callback.hpp>
#include <lexy/dsl.hpp>

#include <fmt/format.h>

#include "detail/LexyLitRange.hpp"

// Grammar Definitions //
namespace ovdl::csv::grammar::windows1252 {
	constexpr auto character = detail::lexydsl::make_range<0x01, 0xFF>();
	constexpr auto control =
		lexy::dsl::ascii::control /
		lexy::dsl::lit_b<0x81> / lexy::dsl::lit_b<0x8D> / lexy::dsl::lit_b<0x8F> /
		lexy::dsl::lit_b<0x90> / lexy::dsl::lit_b<0x9D>;
	constexpr auto id_head = lexy::dsl::ascii::alpha_underscore;
	constexpr auto id_tail = lexy::dsl::ascii::alpha_digit_underscore;

#include "Grammar.inc"
}

namespace ovdl::csv::grammar::utf8 {
	constexpr auto character = lexy::dsl::unicode::character;
	constexpr auto control = lexy::dsl::unicode::control;
	constexpr auto id_head = lexy::dsl::unicode::xid_start_underscore;
	constexpr auto id_tail = lexy::dsl::unicode::xid_continue;

#include "Grammar.inc"
}