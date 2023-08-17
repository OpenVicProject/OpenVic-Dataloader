#pragma once

#include "openvic-dataloader/v2script/Parser.hpp"

namespace ovdl::v2script::warnings {
	inline const ParseWarning make_utf8_warning(const char* file_path) {
		constexpr std::string_view message_suffix = "This may cause problems. Prefer Windows-1252 encoding.";

		std::string message;
		if (!file_path) {
			message = "Buffer is a UTF-8 encoded string. " + std::string(message_suffix);
		} else {
			message = "File '" + std::string(file_path) + "' is a UTF-8 encoded file. " + std::string(message_suffix);
		}

		return ParseWarning { message, 1 };
	}
}

namespace ovdl::ovscript::warnings {
}