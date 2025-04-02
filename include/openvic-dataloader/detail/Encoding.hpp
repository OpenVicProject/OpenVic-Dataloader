#pragma once

#include <cstdint>

namespace ovdl::detail {
	enum class Encoding : std::uint8_t {
		Unknown,
		Ascii,
		Utf8,
		Windows1251,
		Windows1252,
		Gbk,
	};
}