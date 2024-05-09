#pragma once

#include <cstdint>

namespace ovdl::detail {
	enum class Encoding : std::int8_t {
		Unknown,
		Ascii,
		Utf8,
		Windows1251,
		Windows1252
	};
}