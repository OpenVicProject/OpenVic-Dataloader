/// Based heavily on https://github.com/hsivonen/chardetng/tree/143dadde20e283a46ef33ba960b517a3283a3d22

#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <optional>
#include <span>
#include <type_traits>
#include <variant>
#include <vector>

#include <openvic-dataloader/detail/Encoding.hpp>

#include <lexy/action/match.hpp>
#include <lexy/callback/constant.hpp>
#include <lexy/dsl.hpp>
#include <lexy/dsl/ascii.hpp>
#include <lexy/dsl/newline.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>

#include "detail/dsl.hpp"

namespace ovdl::encoding_detect {
	using cbyte = char;
	using ubyte = unsigned char;

	using Encoding = detail::Encoding;

	struct DetectAscii {
		// & 0b10000000 == 0b00000000
		static constexpr auto rule = lexy::dsl::while_(lexy::dsl::ascii::character) + lexy::dsl::eol;
		static constexpr auto value = lexy::constant(true);
	};

	template<bool IncludeAscii>
	struct DetectUtf8 {
		struct not_utf8 {
			static constexpr auto name = "not utf8";
		};

		static constexpr auto rule = [] {
			constexpr auto is_not_ascii_flag = lexy::dsl::context_flag<DetectUtf8>;

			// & 0b10000000 == 0b00000000
			constexpr auto ascii_values = lexy::dsl::ascii::character;
			// & 0b11100000 == 0b11000000
			constexpr auto two_byte = dsl::lit_b_range<0b11000000, 0b11011111>;
			// & 0b11110000 == 0b11100000
			constexpr auto three_byte = dsl::lit_b_range<0b11100000, 0b11101111>;
			// & 0b11111000 == 0b11110000
			constexpr auto four_byte = dsl::lit_b_range<0b11110000, 0b11110111>;
			// & 0b11000000 == 0b10000000
			constexpr auto check_bytes = dsl::lit_b_range<0b10000000, 0b10111111>;

			constexpr auto utf8_check =
				((four_byte >> lexy::dsl::times<3>(check_bytes)) |
					(three_byte >> lexy::dsl::times<2>(check_bytes)) |
					(two_byte >> lexy::dsl::times<1>(check_bytes))) >>
				is_not_ascii_flag.set();

			return is_not_ascii_flag.template create<IncludeAscii>() +
				   lexy::dsl::while_(utf8_check | ascii_values) +
				   lexy::dsl::must(is_not_ascii_flag.is_set()).template error<not_utf8> + lexy::dsl::eof;
		}();

		static constexpr auto value = lexy::constant(true);
	};

	extern template struct DetectUtf8<true>;
	extern template struct DetectUtf8<false>;

	template<typename Input>
	constexpr bool is_ascii(const Input& input) {
		return lexy::match<DetectAscii>(input);
	}

	template<typename Input>
	constexpr bool is_utf8_no_ascii(const Input& input) {
		return lexy::match<DetectUtf8<false>>(input);
	}

	template<typename Input>
	constexpr bool is_utf8(const Input& input) {
		return lexy::match<DetectUtf8<true>>(input);
	}

	struct DetectorData {
		static constexpr std::array latin_ascii = std::to_array<ubyte>({
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 0, 0, 0, 0,			  //
			0, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141, 142, 143, //
			144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 0, 0, 0, 0, 0,		  //
			0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,						  //
			16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 0, 0, 0, 0, 0,					  //
		});

		static constexpr std::array non_latin_ascii = std::to_array<ubyte>({
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,								  //
			100, 100, 100, 100, 100, 100, 100, 100, 100, 100, 0, 0, 0, 0, 0, 0,			  //
			0, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, //
			129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 129, 0, 0, 0, 0, 0,		  //
			0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,								  //
			1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0,								  //
		});

		static constexpr std::array windows_1251 = std::to_array<ubyte>({
			131, 130, 0, 2, 0, 0, 0, 0, 0, 0, 132, 0, 133, 130, 134, 135,					//
			3, 0, 0, 0, 0, 0, 0, 0, 255, 0, 4, 0, 5, 2, 6, 7,								//
			0, 136, 8, 140, 47, 130, 46, 47, 138, 49, 139, 49, 50, 46, 48, 141,				//
			49, 50, 137, 9, 2, 49, 48, 46, 10, 47, 11, 48, 12, 130, 2, 13,					//
			142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, //
			158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, //
			14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,					//
			30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,					//
		});

		static constexpr std::array windows_1252 = std::to_array<ubyte>({
			0, 255, 0, 60, 0, 0, 0, 0, 0, 0, 156, 0, 157, 255, 185, 255,					//
			255, 0, 0, 0, 0, 0, 0, 0, 0, 0, 28, 0, 29, 255, 57, 186,						//
			0, 62, 60, 60, 60, 60, 59, 60, 60, 62, 60, 59, 63, 59, 61, 60,					//
			62, 63, 61, 61, 60, 62, 61, 59, 60, 61, 60, 59, 62, 62, 62, 62,					//
			158, 159, 160, 161, 162, 163, 164, 165, 166, 167, 168, 169, 170, 171, 172, 173, //
			188, 174, 175, 176, 177, 178, 179, 63, 180, 181, 182, 183, 184, 188, 188, 27,	//
			30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45,					//
			60, 46, 47, 48, 49, 50, 51, 63, 52, 53, 54, 55, 56, 60, 60, 58,					//
		});

		// clang-format off
		static constexpr std::array cyrillic = std::to_array<ubyte>({
					 0,  0,  0,  0,  1,  0, 16, 38,  0,  2,  5, 10,121,  4, 20, 25, 26, 53,  9,  5, 61, 23, 20, 26, 15, 95, 60,  2, 26, 15, 25, 29,  0, 14,  6,  6, 25,  1,  0, 27, 25,  8,  5, 39, //  ,
					 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // a,
			 0,  0,  0,255,  0,  0,255,  0,255,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  0,  0,255,  0,  0, // ѓ,
			 0,  0,255,  0,  0,  0,  0,  0,255,255,255,255,  0,255,  2,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, // ђ,
			 0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,  0,255,  0,  0,  0,  0,  0,  4,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, // љ,
			 0,  0,  0,  0,  0,  0,  0,  0,255,255,255,  0,  0,255,  5,  0,  0,  0,  0,  2,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, // њ,
			 0,  0,255,  0,  0,  0,  0,  0,255,  0,255,255,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  1,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, // ћ,
			 0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,255,255,255, // џ,
			 7,  0,  0,255,255,255,255,255,  0,  1,  0,255,255,255, 15,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  1,  0,  0,  0,  1, // ў,
			12,  0,  0,255,255,  0,255,255,  0,  2,  0,  0,  0,  0,  2,  3, 15,  5,  5,  0,  0,  4,  0,  0, 21, 15, 10, 17,  0,  6, 14,  4,  6,  0,  3,  1,  8,  1,  0,  0,  0,  2,  0,  0,  0,  0, // і,
			 0,  0,255,255,255,255,255,255,  0,  0,  0,255,255,  0,  4,  0,  2,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // ё,
			 6,  0,  0,255,255,255,255,255,  0,  0,255,  5,255,  0,  1,  7,  0,  3,  2,  0,  0,  0,  0,  0,  0,  2,  2,  2,  0,  0,  2,  2,  5,  0,  0,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // є,
			12,  0,  0,  0,  0,  0,  0,  0,255,  0,255,  0,  0,  0,  5,  1,  0,  0,  0,  2,  0,  0, 20,255,  0,  0,  0,  0, 15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255,  0,255,255,255,255, // ј,
			 9,  0,  0,255,255,255,255,255,255,  5,255,  0,  0, 13,  3,  3,  0,  4,  1,  0,  1,  2,  0,  0,  0,  1,  0,  0,  4,  0,  0,  1,  3,  0,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // ї,
			32,  0,  0,  2,  2,  2,  0,  0,  0,  1,  0,  0, 28,  0, 23, 22, 26, 22, 19,  0,  3, 12,  5,  0, 44, 38, 18, 58,  1, 21, 44, 17, 54,  1,  2, 28,  5,  8,  3,  1,  9,  0, 12,  0,  0,  0, // а,
			40,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  7,  0,  0,  0,  1,  7,  0,  1,  1,  0,  0,  7,  4,  1,  9,  0,  1,  0,  1,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, // б,
			31,  0,  0,  0,  0,  0,  0,  0,  0, 11,  0,  3,  0,  0, 19,  0,  0,  1,  1,  6,  0,  2,  6,  0,  1,  0,  1,  0, 32,  0,  2,  2, 23,  9,  0,  0,  0,  1,  0,  0,  1,  1,  0,  3,  0,  2, // в,
			23,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  7,  0,  1, 20,  0,  0,  1,  0,  9,  0,  0,  9,  7,  0,  5,  2, 18, 11,  0,  8,  3,  2,  3,  0,  0,  0,  0,  0,  0,  0,  3,  0, 13,  0,  3, // г,
			26,  0,  0,  0,  0,  0,  0,  0,  0,  9,  0,  2,  0,  2, 19,  0,  1,  5,  0, 13,  2,  2,  3,  2,  0,  6,  1, 12, 30,  0,  4,  0,  0,  7,  0,  0,  0,  0,  0,  0,  1,  0,  0,  5,  0,  1, // д,
			12,  0,  0,  1,  4,  5,  0,  0,  0,  0,  0,  0, 24,  1,  5,  7, 11,  3, 12,  1,  6,  6, 11,  0,  3, 15, 14, 14,  4,  8, 25, 14, 29,  0,  1,  1,  4,  8,  8,  2,  0,  3,  1,  0,  0,  0, // е,
			 6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  1,  2,  2,  0,  0,  0,  0,  0,  3,  2,  1,  2,  0,  2,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0, // ж,
			19,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  1,  6,  0,  0,  0, 11,  8,  0,  0,  8,  0,  0,  0,  0,  0,  4,  0,  1,  0,  0,  3,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  1, // з,
			24,  0,  0,  0,  0,  1,  5,  0,  0,  0,  0,  0,  1,  0,  1, 10, 16, 21, 22,  0,  6,  5,  6,  1, 15, 15,  8, 38,  2,  4, 27,  9, 15,  0,  3,  8, 12,  7,  6,  1,  0,  0,  0,  0,  0,  0, // и,
			 6,  0,  0,  0,255,255,255,255,  0,  7,  0,  0,255,  4, 21,  0,  0,  0,  0,  5,  0,  0, 39,  0,  0,  0,  0,  0,  9,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  5,  0,  3,  0,  0, // й,
			54,  0,  0,  0,  0,  0,  0,  0,  1,  8,  0,  0,  0,  0, 10,  0,  1,  0,  1, 11,  0,  0, 12,  0,  1,  2,  0,  4,  8,  0,  2, 23,  2,  4,  0,  2,  3,  3,  8,  0,  0,  3, 16,  1,  4,  3, // к,
			12,  0,  0,  0,  0,  0,  0,  0,  2,  6,  0,  6,  0,  4, 29, 12,  4,  5,  2, 18,  0,  0, 17,  4,  5, 11,  0,  0, 21,  2,  3,  4,  1, 15,  1,  0,  0,  0,  0,  0,  4,  3,  2, 12,  0,  2, // л,
			23,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  4,  0,  0, 17,  1,  0,  0,  0,  7,  0,  1, 13,  2,  0,  0,  0,  0, 13,  0,  2,  4,  0,  2,  0,  0,  0,  0,  0,  0,  1,  4,  2,  4,  1,  1, // м,
			42,  0,  0,  0,  0,  0,  0,  0,  4, 12,  6,  7,  1,  7, 76,  0, 22,  1,  4, 27,  1,  3, 34, 30,  0,  7,  1, 13, 24,  1,  3,  5,  3,  4,  0,  1,  0,  4,  1,  0,  2, 18,  7, 16,  0,  4, // н,
			37,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  1,  0,  1, 10, 27, 22, 15,  1,  2,  3,  7,  5, 32, 11,  7, 38,  8, 21, 24, 11, 23,  0,  2, 10,  2,  2,  3,  2,  0,  0,  1,  0,  0,  0, // о,
			47,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  4,  0,  1,  0,  0,  2,  0,  1,  2,  4,  0,  0,  2,  0,  6,  0,  0,  5,  0,  2,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0, // п,
			19,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  8,  0,  5, 47,  4,  6,  6,  5, 23,  0,  0,  5,  2,  6,  0,  0,  0, 23, 22,  0,  1, 14,  9,  1,  0,  1,  0,  0,  0,  7,  2,  8, 16,  0,  3, // р,
			53,  0,  0,  0,  0,  0,  0,  0,  4,  9,  2,  0,  1,  2, 21,  1,  4,  1,  2, 11,  0,  0, 12,  2,  4,  7,  1, 13, 15,  1,  4,  6,  3,  6,  0,  0,  0,  0,  0,  0,  1,  2,  3,  5,  0,  1, // с,
			28,  0,  0,  0,  0,  0,  0,  0,  1,  6,  0,  1,  0,  1, 32,  0,  1,  3,  0, 12,  0,  1, 22,  1,  4,  7,  1,  6, 23,  0, 14, 41, 14,  3,  0,  1,  1,  1, 21,  0,  2,  2,  6,  2,  1,  4, // т,
			15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  2,  4,  2,  4,  6,  3,  0,  2,  0,  0,  6,  5,  6,  3,  0,  3,  7,  4,  7, 18,  1,  6,  0,  2,  0,  0,  0,  0,  0,  0,  1,  0, // у,
			 8,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // ф,
			41,  0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  1,  0,  2, 30,  0,  2,  0,  0, 11,  0,  0,  5,  1, 14,  3,  0,  3,  6,  0,  7,  0,  0,  1,  0,  1,  0,  2,  0,  0,  0,  4,  3,  5,  0,  0, // х,
			 8,  0,  0,  0,  0,  0,  0,  0,  0,  6,  0,  0,  0,  0,  7,  0,  0,  0,  0,  4,  0,  0,  7,  1,  0,  1,  0,  2,  1,  0,  0,  9,  0,  0,  0,  0,  2,  0,  0,  0,  0,  1,  0,  0,  1,  1, // ц,
			 6,  0,  0,  0,  0,  0,  0,  0,  0,  5,  0,  0,  0,  0,  5,  0,  1,  5,  0,  2,  0,  0,  6,  0,  0,  1,  0,  0,  3,  0,  2,  0,  0,  2,  0,  1,  0,  0,  3,  0,  0,  2,  0,  0,  0,  0, // ч,
			12,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0, 17,  0,  0,  1,  0,  2,  0,  0, 26,  0,  0,  0,  0,  0, 22,  2,  6,  0,  0,  5,  0,  0,  0,  0,  2,  0,  0,  1,  0,  0,  0,  0, // ш,
			 2,  0,255,  0,255,255,255,255,255,  0,  0,  0,255,  0,  1,  1,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  1,  0, // щ,
			 0,  0,255,255,255,255,  0,255,  0,  0,  0,255,255,255,  0,  3,  4,  0,  2,  0,  0,  0,  0,  0, 11,  0,  1,  0,  0,  2,  2,  5,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // ъ,
			 1,  0,  0,255,255,255,255,255,  0,  0,  0,  0,  0,255,  0,  3, 11,  0,  4,  0,  2,  1,  0,  0,  0,  3,  1, 16,  0,  0, 22,  2, 10,  0,  0,  0,  8,  6,  3,  0,  0,  0,  0,  0,  0,  0, // ы,
			 0,  0,  0,255,255,  0,  0,  0,255,  0,  0,  0,  0,  0,  5,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0, 15,  0,  7,  3,  0,  1, 13,  7,  7,  0, 35,  6,  0,  0,  0,  0,  0,  0,  0,  6,  0, // ь,
			10,  0,  0,255,255,255,255,255,  0,  0,  0,  0,255,  0,  0,  1,  1, 10, 11,  0,  2,  2,  0,  0,  0,  9,  3,  9,  0,  0,  7,  6,  9,  0,  0,  8,  3,  2,  1,  0,  0,  0,  0, 17,  0,  0, // э,
			14,  0,  0,  0,255,255,255,255,  0,  0,  0,  0,255,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  2,  0,  0,  2,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // ю,
			5,  0,  0,255,255,255,255,255,  0,  9,  0,  0,255,  0, 11,  0,  3,  0,  0,  0,  0,  2, 24,  0,  0,  5,  2, 14,  1,  0,  2,  3,  1,  0,  0,  1,  3,  0,  0,  0,  0, 16,  1,  0,  0,  0,  // я,
		//   ,  a,  ѓ,  ђ,  љ,  њ,  ћ,  џ,  ў,  і,  ё,  є,  ј,  ї,  а,  б,  в,  г,  д,  е,  ж,  з,  и,  й,  к,  л,  м,  н,  о,  п,  р,  с,  т,  у,  ф,  х,  ц,  ч,  ш,  щ,  ъ,  ы,  ь,  э,  ю,  я,
		});
		// clang-format on

		// clang-format off
		static constexpr std::array western = std::to_array<ubyte>({
																													   18,  3,  0,254, 74,  0,  5,254,254,  2, 25,254,149,  4,254, 66,148,254,  0,254,122,238,  8,  1, 20, 13,254, 35, 20,  3,  1,  0, //  ,
																													    0,  3,  0,  0,  0,  0,  0,  5,  2,  0, 86,  9, 76,  0,  0,  0,241,  0,  0, 49,  0,  0,  0,  0, 11,  2,  0, 34,  0,  1,  2,  0, // a,
																													   19,  0,  0,  5,  5,  0,  0,  8, 13,  5,  0, 34, 22,  0,  0,  0,  4,  0,  0,  0,  6,  1,  3,  3, 42, 37,  8,  8,  0, 67,  0,  0, // b,
																													    0,  0,  0,  9,  6,  1,  0, 22, 10,  1,  0, 19, 54,  1,  0,  1, 18,  3,  1,  2, 40,  7,  0,  0,  6,  0,  3,  5,  1, 34,  0,  0, // c,
																													    0,  0,  0,  5,  5,  0,  0, 12, 45, 16,  1,  6, 42,  0, 13,  3, 10,  0,  2,  0, 66, 11,  5,  8, 33,104,  3,  4,  0, 19,  0,  0, // d,
																													   63,  5,  0,  0,  0,  0,  2, 33, 15,  1,  3,  0, 87,  0,  0,  0,  0,  0,  1, 21,  0,  0,  0, 49,  1, 11,  0,  3,  0,  9,  1,  0, // e,
																													    0,  0,  0,  8,  8,  0,  0, 10,  2,  7,  0,162, 23,  0, 13,  0,  4,  0,  0,  0,  1,  3,  0,  0, 15,  4,  0,  0,  0,  4,  0,  0, // f,
																													    1,  0,  0, 14, 16, 24,  0, 29, 11, 41,  0, 13, 86,  0, 14,  9,  3,  0,  0,  0, 20,  8,  7,  7, 13, 37, 14,  0,  0, 12,  0,  0, // g,
																													    1,  0,  0,  0,  0,  0,  0, 47,  2,  0,  0,  0,  1,  0,  7,  0,  0,  0,  0,  0,  0,  0,  0, 29, 20,  0,  0,  0,  0, 45,  0,  0, // h,
																													    5,  4,  0,166,120,  0,  0,144,  0,  2,  3, 88,254,  0,  0,  0,  0,  0,  0,  3, 28,107,  0,112,  8,  2, 44, 32,  0,  3,  3,  0, // i,
																													    0,  0,  0,  0,  0,  0,  0, 39,  9,  0,  0,  2,  1,  0,  2,  0,  0,  0,  0,  4,  0,  0,  0, 16, 18, 44,  0,  0,  0,  0,  0,255, // j,
																													    0,  2,  0,  0,  1,  0,  0, 48, 31, 32,  1, 60,  1,  0,  4,  0,  1,  0,  0,  0,  1,  3,  0,  2, 20, 47,  0,  0,  0, 20,  0,  0, // k,
																													    4,  0,  0, 12, 16,  0,  0, 54, 40, 48,  0, 64, 36,  0, 39,  6, 12,  3,  0,  0, 27,  9,  3, 24, 42, 33,  2,  9,  7, 77,  0,  0, // l,
																													    0,  0,  0, 14,  5,  4,  0, 60, 11,  4,  3, 48, 30,  7, 28,  1, 10,  1,  0,  0, 24, 41,  3,  3, 19, 24,  1,  8,  2, 36,  0,  0, // m,
																													    1,  1,  0, 24, 91, 16,  0,132, 62, 73,  1, 56, 71, 33, 78,  7, 35,  2,  3,  0, 94,254, 10, 21, 33, 38, 24, 21,  1, 61,  0,  0, // n,
																													    0,  1,  0,  0,  0,  0,254,  6,  0,  1, 27,  0, 13,  0,  0, 84,127,  0,  0, 62,  0,  1,  0,  0,  2,  0,  0,  0,  0,  0,  0,  0, // o,
																													    0,  0,  0,  5,  2,  0,  0,  9, 15,  0,  0,  4, 34,  0,  6,  0,  6,  0,  0,  0, 20, 12,  9, 28, 10, 22,  0,  3,  0,  7,  0,  0, // p,
																													    0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1, 33,  1,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,255,255, // q,
																													    0,  0,  0, 83, 62,  1,  0,198,139,125,  0,229, 94, 54,190, 38, 18,  1,  0,  0,176, 24, 16, 29,193,181, 13, 13,  2,131,  0,  0, // r,
																													    1,  0,  0, 41, 34,  0,  0, 41, 24, 42,  0, 68,113, 15,159,  6, 43, 19,  4, 58, 14, 18,  1,  4, 48, 42,  4, 12,  9, 20,  0,  0, // s,
																													    7,  1,  0, 14, 20,  8,  0, 56, 37, 31,  0,104, 67, 14,113,  3, 50,  9,  5,  0, 89,  7, 19, 22, 13, 14, 40, 12, 15, 18,  0,  0, // t,
																													    0,  1,  5,  1,  2,  0,  0, 30,  0,  0,  1, 15,  2,  0,  1,  0,  1,  0,  0,  2,  4,  0,  0, 36,  0,  0,  0,  0,  0,  0,  0,  0, // u,
																													    0,  2,  0,  1,  6,  0,  0, 29, 33, 13,  0, 19, 46,  0, 15,  0,  7,  0,  1, 31,  2,  2,  3,  1, 32, 27,  0,  0,  1,  1,  0,  0, // v,
																													    0,  0,  0,  0,  0,  0,  0,  0,  2,  0,  0,  3,  0,  0,  4,  0,  0,  0,  0,  0,  0,  2,  0,  0,  1,  0,  0,  0,  0,  0,  0,255, // w,
																													    0,  0,  0,  1, 16,  0,  0, 23,  0,  0,  0,  3, 14,  0,  0,  0,  2,  3,  0,  0,  0,  6,  0,  0,  0,  0,  0,  0,  0,  0,255,  0, // x,
																													    0,  0,  0,  0,  0,  0,  0, 58,  8,  0,  0,  1,  1, 62,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  6, 82,  0,  0,  0,  0,  0,255, // y,
																													    0,  0,  0,  0,  2,  0,  0,  0, 14,  0,  0,  7,  3,  0,  6,  0,  3,  5,  0,  0,  0,  0,  4,  0,  1,  0,  0,  0,  0,  0,  0,  0, // z,
			0, 29,  0,  0,  0, 15,  0,  0,  0, 11,  0,  0,  0,  0,  0, 20,  0,  0,  0,  0,  0, 37,  0,  0,  0,  0,  0,  0,255,255,  0,  0,255,255,  4,  0,  0,255,255,  0,255,  0,255,  0,  0,255,255,255,  0,  0,  0,  8,  0,255,  0,  0,  2,  0,  0, // ß,
			6,  2,  0,  0,  0,  1,  0,  0,  0,  1,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0, 10,  1,  0,  0,  0,  0,  0,  0,  0,255,  0,  1,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, // š,
			3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,255,255,255,  0,  0,  0,255,255,255,  0,255,255,255,255,  0,  0,255,255,255,255,255,255,  0,255,255,255,  0,255,255, // œ,
		  107,  0, 22, 16, 18, 14,  6, 24, 46, 15,  2,  0, 42, 18, 17,  0, 36,  0, 34,  4,254,  1,  2,  0,  0,  1,  0,  0,  0,255,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,255,255,255,255,255,  0,  0,255,  0,  0,  0, // à,
		   41,  0, 10,  8, 21, 34,  5,  5, 60, 18,  5,  1, 29, 42, 26,  2, 16,  0, 27,  9, 43, 28,  7,  0,  0,  1,  4,  0,  0,255,  0,  0,255,255,255,  0,255,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,  0,  0,  0,  0,  0,255, // á,
		   24,  0,  1,  2,  0,  0,  0,  0,  7,  0,  0,  0,  3,  1,  0,  0,  0,  0,  2,  0,  5,  0,  1,  0,  0,  0,  0,255,  0,255,  0,  0,  0,255,  0,255,  0,  0,  0,  2,  0,255,  0,255,  0,  0,  0,  0,255,  0,255,255,255,255,255,  0,255,  0,255, // â,
			0,  0,  0,  1,  2,  3,  0,  1,  2, 12,  0,  0,  1,  7, 29,  4,  1,255, 11, 66, 11,  0,  1,  0,  0,  0,  0,255,  0,255,255,255,  0,  0,  0,255,255,127,255,255,255,255,255,  0,  0,255,  0,  0,255,255,  0,255,255,255,255,255,255,255,255, // ã,
		  134,  1, 11,  0, 25,  6, 15, 11, 61, 24,123, 95,114, 68, 53,  1, 49,  0, 60, 98,198,  0, 88, 29,  0,  6, 12,  0,  0,255,  0,255,  0,  0,118,  0,255,  0,255,  0,255,  0,255,  0,255,255,  0,255,255,  0,255,  2,255,255,255,  0,  0,  0,255, // ä,
		  156,  0, 12, 14, 19,  3, 12, 47, 17,  3, 12,  5, 30, 47, 22,  0,205,  0,184, 70, 19,  0, 22,  8,  0,  6,  1,255,  0,255,255,  0,255,  0,  0,  0,  0,  0,255,  0,255,  0,255,  0,  0,255,255,255,255,255,255,  0,  0,255,255,255,255,255,255, // å,
		   26,  0,  7,  0,  4,  0, 23,  8, 15,  0, 18, 19, 56, 23, 24,  0,  9,  0, 82, 37, 24,  0, 71,  0,  0,  0,  0,255,  0,255,255,  0,255,255,  0,  0,  0,  0,255,  0,255,255,255,  0,255,255,  0,255,255,255,255,  0,  0,255,255,255,255,  0,255, // æ,
		   17,112,  0,  2,  0, 15,  0,  0,  0, 35,  0,  0,  2,  0, 59,  9,  1,  0, 36,  0,  0,  8,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,255, // ç,
		  254,  0,  9, 14, 20,  0, 15,  6, 70,144, 14, 45, 47, 92, 16,  3,123,  0, 38, 23,115, 52, 22, 42,  2, 80, 19,255,  0,255,  0,  0,255,255,  0,255,255,  0, 10,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,255,255,255,  0,  0,  0,  1,255,255, // è,
		  152,  2, 19, 24, 85,  0, 29, 23, 26, 25,  2,  9, 43, 60, 62,  1, 32,  0,122, 45,169, 15, 13, 30,  7,  4,  8,  0,  0,255,  0,  0,  0,  0,  0,255,  0,  0,  0,  2,  0,  0,  0,  0,  0,  0,  1,255,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0, // é,
			5,  0,  0,  3,  7,  0,  0, 10,  2,  3,  0, 26,  6,  6, 20,  1,  2,  0, 20,  1, 11,  5,  5,  2,  0,  0,  1,255,  0,255,255,255,  0,255,255,255,255,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,255,  0,  0,255,255,255,  0,255,  0,  0,  0,255, // ê,
		   36,  2, 23, 15, 36,143,  5, 23, 52, 52, 66, 48, 92, 57,216, 10,125, 35, 89, 58,254,  9, 24, 14,  0,  0,  8,255,  0,255,  0,255,255,255,  0,  0,255,  1,  0,  0,  0,  0,  0,255,  0,  0,  0,255,255,255,  0,  0,  0,  0,255,  0,  0,  0,255, // ë,
		   12,  0,  1,  4,  6,  0,  3, 21, 10,  0,  0,  0, 18,  8,  4,  0,  1,  0, 65, 35,  8,  3,  0,  0,  0,  0,  0,255,  0,255,  0,  0,255,255,255,255,255,255,  0,  0,  0,255,  0,  0,  0,255,  0,  0,255,  0,255,255,255,  0,255,255,  0,  0,255, // ì,
		   40, 72,  7, 10, 16,  2, 23, 10, 34,  0,  0,  1, 34, 15, 21,  1,  3,  0,203, 28, 58, 23, 11,  0, 10,  0,  2,  0,  0,  0,  0,  0,  0,255,  0,255,255,  0,  0,  0,  0,255,  0,  0,255,255,  1,255,  0,255,255,  0,255,255,  0,255,  2,  0,255, // í,
			6,  5,  1,  9,  5,  0,  0,  0, 22,  0,  9,  8,  8,  6,  9,  1, 10,  0, 20,  6,182,  0, 13,  0,  0, 24,  1,255,  0,255,255,255,  0,  0,255,  0,255,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,255,255,255,255,255,  0,255,255,255, // î,
			0,  6,  0,  0,  0,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  3,  0,  0,  0,  0,  0,  9,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,255,  0,255,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,255,  0,  0,  0,  0,255,255,  0,  0,  0,255,255, // ï,
			0,254,  0,  0,  0, 26,  0,  0,  0, 61,  0,  0,  0,  0,  0, 14,  0,  0,  0,  0,  0, 25,  0,  0,  0,  0,  0,255,255,255,  0,  0,  0,  0,  0,  0,255,255,  0,  0,  0,255,  0,  1,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,255,  0,255,255, // ñ,
		   20,  0, 56, 43,  8,162, 14,  3, 23, 19,  2,118, 31, 26, 46,  0, 20,  0, 23,  6, 24, 19,  6, 21,  5, 27, 63,255,  0,255,  0,  0,255,255,255,255,255,  3,  0,255,255,255,  0,  0,255,  0,  0,  0,  0,255,  0,255,255,  0,255,255,  0,255,255, // ò,
		   67,  0, 12, 15,  9,  7,  8, 66, 13,254,  3, 23, 14, 16, 16,  0,  8,  0, 29, 11, 26,  0,  5,  5,  1, 10, 13,255,  0,255,255,  0,255,  0,  0,255,255,  1,255,  0,255,255,  0,  0,255,  0,  1,  0,  0,  0,  0,255,255,255,  0,255,255,  0,255, // ó,
		   18,  3,  3, 12,  1,  0,  2,  0,  7,  0,  1,  0,  2,  2,  8,  0,  6,  0,  6,  7,  4,  0,  2,  0,  0,  0,  1,255,  0,  0,255,  0,  0,255,255,255,  0,  0,  0,  0,  0,255,255,  0,  0,  0,  0,  0,  0,  0,255,255,255,255,  0,  0,255,255,255, // ô,
		   29,  2,  0,  0,  0,  0,  0,  0,  5,  2, 22, 30, 25, 38, 19,  0, 33,255,  4, 39, 24,  0, 88,  0,  0,  0,  0,255,  0,255,255,  0,255,  0,255,255,255, 36,255,255,255,255,255,  0,255,255,  0,255,  0,  0,  6,  0,255,255,255,  0,  0,  0,255, // õ,
		   44,  0, 33,  0, 25,  0,142,  5, 46, 10, 25, 32, 26, 13,  6,  0,  3,  0, 30,  8, 35,  0, 25,  5,  0, 44,  7,  0,  0,255,255,  0,255,255, 73,  0,255,  0,  0,  0,255,255,255,255,255,  0,  0,255,  0,  0,  0, 39,  0,255,255,255,  0,  0,  0, // ö,
		   52,  0, 21,  0, 57,  0,119, 12, 47,  3, 59, 33, 45, 15, 12,  0,  3,  0, 52, 82, 49,  1, 11,  0,  0,  0,  0,  0,255,  0,255,255,255,255,255,  0,  0,  0,255,  0,255,255,255,  0,255,255,  0,255,255,255,255,  0,  0,255,255,255,255,255,  0, // ø,
		   25,  0,  4,  3, 53,  0,  0,  2, 12, 72,  0,  0, 30,  0,  0,254,  0,  0,  6,  3,  3,  0,  0,  0,  0,  0,  0,255,  0,255,  0,255,  0,255,255,255,255,  0,  0,  0,  0,255,  0,255,255,255,255,  0,255,  0,  0,255,255,  0,  0,  0,  0,  0,  0, // ù,
		   19,  2,  1,  7,  9,  1, 12,  5,  9, 41,  1,  0, 10,  7,  9,  0,  8,  0, 12, 28,  8,  0,  0,  0,  0,  1,  0,255,  0,255,255,  0,255,255,255,255,  0,  0,255,  0,255,255,255,  0,255,255,  0,  0,  0,255,  0,255,255,  0,  0,255,255,  0,255, // ú,
			0,  0,  0,  0,  1,  5,  0,  0,  1,  0,  0,  0,  0,  0,  0, 45,  0,  0,  3,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,255,255,255,  0,255,255,255,255,  0,255,  0,255,255,255,  0,  0,255,255,255,255,  0,255,255,255,  0,255,  0,  0,255,  0, // û,
		   95,  2, 19,  0,  6,  2,121,  9, 15,  1,  5, 44, 18, 26,  7,  0, 11,  2, 68, 49, 20,  0,  2, 17,  0,  0,  6,  0,  0,255,  0,255,255,255,  0,255,255,  0,255,  0,255,  0,255,255,255,  0,  0,255,255,255,  0,  0,255,  0,  0,  0, 31,  0,  0, // ü,
			1,  1,  0,  0,  2,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,255,255,  0,  0,255,  0,255,  0,255,255,255,255,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,255, // ž,
			0,  0,  0,  0,  0,  0,255,  0,  0,255,  0,  0,  0,  0,  0,  0,  0,255,  0,  0,  0,  0,  0,  0,255,255,  0,255,255,255,255,255,255,  0,255,  0,255,255,255,255,255,255,255,255,255,255,255,255,255,  0,  0,255,  0,255,255,255,  0,  0,  0, // ÿ,
		//   ,  a,  b,  c,  d,  e,  f,  g,  h,  i,  j,  k,  l,  m,  n,  o,  p,  q,  r,  s,  t,  u,  v,  w,  x,  y,  z,  ß,  š,  œ,  à,  á,  â,  ã,  ä,  å,  æ,  ç,  è,  é,  ê,  ë,  ì,  í,  î,  ï,  ñ,  ò,  ó,  ô,  õ,  ö,  ø,  ù,  ú,  û,  ü,  ž,  ÿ,

		});
		// clang-format on
	};

	namespace class_size {
		constexpr std::size_t cyrillic_ascii = 2;
		constexpr std::size_t cyrillic_non_ascii = 44;
		constexpr std::size_t western_ascii = 27;
		constexpr std::size_t western_non_ascii = 32;
	}

	constexpr std::size_t ASCII_DIGIT = 100;

	struct ByteScore {
		const Encoding encoding;
		const std::array<ubyte, 128>& lower;
		const std::array<ubyte, 128>& upper;
		const std::span<const ubyte> probabilities;
		const std::size_t ascii;
		const std::size_t non_ascii;

		static inline constexpr std::optional<std::size_t> compute_index(std::size_t x, std::size_t y, std::size_t ascii_classes, std::size_t non_ascii_classes) {
			if (x == 0 && y == 0) {
				return std::nullopt;
			}

			if (x < ascii_classes && y < ascii_classes) {
				return std::nullopt;
			}

			if (y >= ascii_classes) {
				return (ascii_classes * non_ascii_classes) + (ascii_classes + non_ascii_classes) * (y - ascii_classes) + x;
			}

			return y * non_ascii_classes + x - ascii_classes;
		}

		inline constexpr cbyte classify(cbyte byte) const {
			cbyte high = byte >> 7;
			cbyte low = byte & 0x7F;
			if (high == 0) {
				return static_cast<cbyte>(lower[low]);
			}

			return static_cast<cbyte>(upper[low]);
		}

		inline constexpr bool is_latin_alphabetic(cbyte caseless_class) const {
			return caseless_class > 0 && caseless_class < (ascii + non_ascii);
		}

		inline constexpr bool is_non_latin_alphabetic(cbyte caseless_class) const {
			return caseless_class > 1 && caseless_class < (ascii + non_ascii);
		}

		inline constexpr int64_t score(cbyte current_class, cbyte previous_class) const {
			constexpr int64_t IMPLAUSABILITY_PENALTY = -220;

			constexpr std::size_t PLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE = 0;
			constexpr std::size_t IMPLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE = 1;
			constexpr std::size_t IMPLAUSIBLE_BEFORE_ALPHABETIC = 2;
			constexpr std::size_t IMPLAUSIBLE_AFTER_ALPHABETIC = 3;
			constexpr std::size_t PLAUSIBLE_NEXT_TO_NON_ASCII_ALPHABETIC_ON_EITHER_SIDE = 4;
			constexpr std::size_t PLAUSIBLE_NEXT_TO_ASCII_ALPHABETIC_ON_EITHER_SIDE = 5;

			std::size_t stored_boundary = ascii + non_ascii;
			if (current_class < stored_boundary) {
				if (previous_class < stored_boundary) {
					if (auto index = compute_index(previous_class, current_class, ascii, non_ascii); index) {
						ubyte b = probabilities[index.value()];
						if (b == 255) {
							return IMPLAUSABILITY_PENALTY;
						}
						return b;
					}
					return 0;
				}

				if (current_class == 0 || current_class == ASCII_DIGIT) {
					return 0;
				}

				std::size_t previous_unstored = previous_class - stored_boundary;
				switch (previous_unstored) {
					case PLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE:
					case IMPLAUSIBLE_AFTER_ALPHABETIC:
						return 0;
					case IMPLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE:
					case IMPLAUSIBLE_BEFORE_ALPHABETIC:
						return IMPLAUSABILITY_PENALTY;
					case PLAUSIBLE_NEXT_TO_NON_ASCII_ALPHABETIC_ON_EITHER_SIDE:
						if (current_class < ascii) {
							return IMPLAUSABILITY_PENALTY;
						}
						return 0;
					case PLAUSIBLE_NEXT_TO_ASCII_ALPHABETIC_ON_EITHER_SIDE:
						if (current_class < ascii) {
							return 0;
						}
						return IMPLAUSABILITY_PENALTY;
					default:
						assert(previous_class == ASCII_DIGIT);
						return 0;
				}
			}

			if (previous_class < stored_boundary) {
				if (previous_class == 0 || previous_class == ASCII_DIGIT) {
					return 0;
				}

				std::size_t current_unstored = current_class - stored_boundary;
				switch (current_unstored) {
					case PLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE:
					case IMPLAUSIBLE_BEFORE_ALPHABETIC:
						return 0;
					case IMPLAUSIBLE_NEXT_TO_ALPHABETIC_ON_EITHER_SIDE:
					case IMPLAUSIBLE_AFTER_ALPHABETIC:
						return IMPLAUSABILITY_PENALTY;
					case PLAUSIBLE_NEXT_TO_NON_ASCII_ALPHABETIC_ON_EITHER_SIDE:
						if (previous_class < ascii) {
							return IMPLAUSABILITY_PENALTY;
						}
						return 0;
					case PLAUSIBLE_NEXT_TO_ASCII_ALPHABETIC_ON_EITHER_SIDE:
						if (previous_class < ascii) {
							return 0;
						}
						return IMPLAUSABILITY_PENALTY;
					default:
						assert(current_class == ASCII_DIGIT);
						return 0;
				}
			}

			if (current_class == ASCII_DIGIT || previous_class == ASCII_DIGIT) {
				return 0;
			}

			return IMPLAUSABILITY_PENALTY;
		}
	};

	enum class ScoreIndex {
		Windows1251,
		Windows1252
	};

	static constexpr std::array byte_scores {
		ByteScore {
			.encoding = Encoding::Windows1251,
			.lower = DetectorData::non_latin_ascii,
			.upper = DetectorData::windows_1251,
			.probabilities = DetectorData::cyrillic,
			.ascii = class_size::cyrillic_ascii,
			.non_ascii = class_size::cyrillic_non_ascii },
		ByteScore {
			.encoding = Encoding::Windows1252,
			.lower = DetectorData::latin_ascii,
			.upper = DetectorData::windows_1252,
			.probabilities = DetectorData::western,
			.ascii = class_size::western_ascii,
			.non_ascii = class_size::western_non_ascii }
	};

	constexpr const ByteScore& get_byte_score(ScoreIndex index) {
		return byte_scores[static_cast<std::underlying_type_t<ScoreIndex>>(index)];
	}

	struct Utf8Canidate {
		std::optional<int64_t> read(const std::span<const cbyte>& buffer);
	};

	struct AsciiCanidate {
		std::optional<int64_t> read(const std::span<const cbyte>& buffer);
	};

	struct NonLatinCasedCanidate {
		enum class CaseState {
			Space,
			Upper,
			Lower,
			UpperLower,
			AllCaps,
			Mix,
		};

		const ByteScore& score_data;
		cbyte prev {};
		CaseState case_state = CaseState::Space;
		bool prev_ascii = true;
		int64_t current_word_len {};
		uint64_t longest_word {};
		bool ibm866 = false;
		bool prev_was_a0 = false;

		std::optional<int64_t> read(const std::span<const cbyte>& buffer);
	};

	struct LatinCanidate {
		enum class CaseState {
			Space,
			Upper,
			Lower,
			AllCaps,
		};

		enum class OrdinalState {
			Other,
			Space,
			PeriodAfterN,
			OrdinalExpectingSpace,
			OrdinalExpectingSpaceUndoImplausibility,
			OrdinalExpectingSpaceOrDigit,
			OrdinalExpectingSpaceOrDigitUndoImplausibily,
			UpperN,
			LowerN,
			FeminineAbbreviationStartLetter,
			Digit,
			Roman,
			Copyright,
		};

		const ByteScore& score_data;
		cbyte prev {};
		CaseState case_state = CaseState::Space;
		uint32_t prev_non_ascii {};
		OrdinalState ordinal_state = OrdinalState::Space; // Used only when `windows1252 == true`
		bool windows1252;

		constexpr LatinCanidate(const ByteScore& data) : score_data(data) {
			windows1252 = data.encoding == Encoding::Windows1252;
		}

		std::optional<int64_t> read(const std::span<const cbyte>& buffer);
	};

	using InnerCanidate = std::variant<NonLatinCasedCanidate, LatinCanidate, Utf8Canidate, AsciiCanidate>;

	template<class... Ts>
	struct overloaded : Ts... {
		using Ts::operator()...;
	};

	template<class... Ts>
	overloaded(Ts...) -> overloaded<Ts...>;

	struct Canidate {
		InnerCanidate inner;
		std::optional<int64_t> score_value;

		template<typename CanidateT>
		static constexpr Canidate create_canidate() {
			return {
				.inner = CanidateT(),
				.score_value = 0
			};
		}

		template<typename CanidateT>
		static constexpr Canidate create_canidate(const ByteScore& score) {
			return {
				.inner = CanidateT { score },
				.score_value = 0
			};
		}

		static constexpr Canidate new_utf8() {
			return create_canidate<Utf8Canidate>();
		}

		static constexpr Canidate new_ascii() {
			return create_canidate<AsciiCanidate>();
		}

		static constexpr Canidate new_latin(ScoreIndex index) {
			return create_canidate<LatinCanidate>(get_byte_score(index));
		}

		static constexpr Canidate new_non_latin_cased(ScoreIndex index) {
			return create_canidate<NonLatinCasedCanidate>(get_byte_score(index));
		}

		constexpr std::optional<int64_t> score(const std::span<const cbyte>& buffer, std::size_t encoding, bool expectation_is_valid) {
			if (auto old_score = score_value) {
				auto new_score = std::visit([&](auto& inner) {
					return inner.read(buffer);
				},
					inner);
				if (new_score) {
					score_value = old_score.value() + new_score.value();
				} else {
					score_value = std::nullopt;
				}
			}

			if (auto nlcc = std::get_if<NonLatinCasedCanidate>(&inner)) {
				if (nlcc->longest_word < 2) {
					return std::nullopt;
				}
			}
			return score_value;
		}

		constexpr Encoding encoding() const {
			return std::visit(
				overloaded {
					[](const Utf8Canidate& canidate) {
						return Encoding::Utf8;
					},
					[](const AsciiCanidate& canidate) {
						return Encoding::Ascii;
					},
					[](const LatinCanidate& canidate) {
						return canidate.score_data.encoding;
					},
					[](const NonLatinCasedCanidate& canidate) {
						return canidate.score_data.encoding;
					} },
				inner);
		}
	};

	struct Detector {
		std::vector<Canidate> canidates {
			Canidate::new_ascii(),
			Canidate::new_utf8(),
			Canidate::new_latin(ScoreIndex::Windows1252),
			Canidate::new_non_latin_cased(ScoreIndex::Windows1251),
		};

		Encoding default_fallback = Encoding::Unknown;

		constexpr std::pair<Encoding, bool> detect_assess(std::span<const cbyte> buffer, bool allow_utf8 = true) {
			int64_t max = 0;
			Encoding encoding = default_fallback; // Presumes fallback, defaults to Unknown encoding if unknown (which skips conversion)
			std::size_t i = 0;
			for (Canidate& canidate : canidates) {
				if (!allow_utf8 && canidate.encoding() == Encoding::Utf8) {
					continue;
				}

				if (auto score = canidate.score(buffer, i, false)) {
					switch (canidate.encoding()) {
						using enum Encoding;
						case Ascii:
						case Utf8:
							return { canidate.encoding(), true };
						default: break;
					}

					auto value = score.value();
					if (value > max) {
						max = value;
						encoding = canidate.encoding();
					}
				}
				i++;
			}
			return { encoding, max >= 0 };
		}

		constexpr Encoding detect(std::span<const cbyte> buffer, bool allow_utf8 = true) {
			return detect_assess(buffer, allow_utf8).first;
		}

		template<typename BufferEncoding>
		std::pair<Encoding, bool> detect_assess(const lexy::buffer<BufferEncoding, void>& buffer, bool allow_utf8 = true) {
			auto span = std::span<const cbyte>(buffer.data(), buffer.size());
			return detect_assess(span);
		}

		template<typename BufferEncoding>
		constexpr Encoding detect(const lexy::buffer<BufferEncoding, void>& buffer, bool allow_utf8 = true) {
			return detect_assess(buffer, allow_utf8).first;
		}
	};
}