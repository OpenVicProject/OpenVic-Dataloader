#pragma once

#include "openvic-dataloader/v2script/Parser.hpp"

namespace ovdl::errors {
	inline const ParseError make_no_file_error(const char* file_path) {
		std::string message;
		if (!file_path) {
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