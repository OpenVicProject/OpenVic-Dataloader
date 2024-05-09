#pragma once

#include <string>
#include <string_view>

namespace ovdl::v2script::warnings {
	inline const std::string make_utf8_warning(std::string_view file_path) {
		constexpr std::string_view message_suffix = "This may cause problems. Prefer Windows-1252 encoding:";

		std::string message;
		if (file_path.empty()) {
			message = "Buffer is UTF-8 encoded. " + std::string(message_suffix);
		} else {
			message = "File is UTF-8 encoded. " + std::string(message_suffix);
		}

		return message;
	}
}

namespace ovdl::ovscript::warnings {
}