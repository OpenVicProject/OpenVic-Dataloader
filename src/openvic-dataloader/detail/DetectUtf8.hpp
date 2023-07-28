#pragma once

#include "detail/LexyLitRange.hpp"
#include <lexy/action/match.hpp>
#include <lexy/dsl.hpp>

namespace ovdl::detail {
	namespace detect_utf8 {

		template<bool INCLUDE_ASCII>
		struct DetectUtf8 {
			struct not_utf8 {
				static constexpr auto name = "not utf8";
			};

			static constexpr auto rule = [] {
				constexpr auto is_not_ascii_flag = lexy::dsl::context_flag<DetectUtf8>;

				// & 0b10000000 == 0b00000000
				constexpr auto ascii_values = lexydsl::make_range<0b00000000, 0b01111111>();
				// & 0b11100000 == 0b11000000
				constexpr auto two_byte = lexydsl::make_range<0b11000000, 0b11011111>();
				// & 0b11110000 == 0b11100000
				constexpr auto three_byte = lexydsl::make_range<0b11100000, 0b11101111>();
				// & 0b11111000 == 0b11110000
				constexpr auto four_byte = lexydsl::make_range<0b11110000, 0b11110111>();
				// & 0b11000000 == 0b10000000
				constexpr auto check_bytes = lexydsl::make_range<0b10000000, 0b10111111>();

				constexpr auto utf8_check =
					((four_byte >> lexy::dsl::times<3>(check_bytes)) |
						(three_byte >> lexy::dsl::times<2>(check_bytes)) |
						(two_byte >> lexy::dsl::times<1>(check_bytes))) >>
					is_not_ascii_flag.set();

				return is_not_ascii_flag.template create<INCLUDE_ASCII>() +
					   lexy::dsl::while_(utf8_check | ascii_values) +
					   lexy::dsl::must(is_not_ascii_flag.is_set()).template error<not_utf8>;
			}();
		};
	}

	template<typename Input>
	constexpr bool is_utf8_no_ascii(const Input& input) {
		return lexy::match<detect_utf8::DetectUtf8<false>>(input);
	}

	template<typename Input>
	constexpr bool is_utf8(const Input& input) {
		return lexy::match<detect_utf8::DetectUtf8<true>>(input);
	}
}