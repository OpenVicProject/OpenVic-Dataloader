#pragma once

#include "openvic-dataloader/v2script/Parser.hpp"

namespace ovdl::v2script::errors {
	inline const v2script::Parser::Error make_no_file_error(const char* file_path) {
		std::string message;
		if (!file_path) {
			message = "File path not specified.";
		} else {
			message = "File '" + std::string(file_path) + "' was not found.";
		}

		return v2script::Parser::Error { Parser::Error::Type::Fatal, message, 1 };
	}
}

namespace ovdl::ovscript::errors {
}