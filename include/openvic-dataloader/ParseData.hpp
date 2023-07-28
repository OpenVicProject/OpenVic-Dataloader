#pragma once

#include <string>

namespace ovdl {
	struct ParseData {
		const std::string production_name;
		const unsigned int context_start_line;
		const unsigned int context_start_column;
	};
}