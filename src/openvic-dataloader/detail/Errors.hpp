#pragma once

#include <string_view>

#include <openvic-dataloader/ParseError.hpp>

namespace ovdl::errors {
	inline const ParseError make_no_file_error(std::string_view file_path) {
		std::string message;
		if (file_path.empty()) {
			message = "File path not specified.";
		} else {
			message = "File '" + std::string(file_path) + "' was not found.";
		}

		return ParseError { ParseError::Type::Fatal, message, 1 };
	}
}

namespace ovdl::v2script::errors {

}

namespace ovdl::ovscript::errors {
}