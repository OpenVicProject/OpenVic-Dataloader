#pragma once

#include <string>

namespace ovdl {
	struct ParseWarning {
		const std::string message;
		const int warning_value;
	};
}