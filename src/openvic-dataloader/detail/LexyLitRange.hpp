#pragma once

#include <lexy/dsl/literal.hpp>

namespace ovdl::detail::lexydsl {
	template<unsigned char LOW, unsigned char HIGH>
	consteval auto make_range() {
		if constexpr (LOW == HIGH) {
			return lexy::dsl::lit_c<LOW>;
		} else if constexpr (LOW == (HIGH - 1)) {
			return lexy::dsl::lit_c<LOW> / lexy::dsl::lit_c<HIGH>;
		} else {
			return lexy::dsl::lit_c<LOW> / make_range<LOW + 1, HIGH>();
		}
	}
}