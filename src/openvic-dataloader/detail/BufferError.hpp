#pragma once

#include <cstdint>

namespace ovdl::detail {
	enum class buffer_error : std::uint8_t {
		success,
		/// An internal OS error, such as failure to read from the file.
		os_error,
		/// The file was not found.
		file_not_found,
		/// The file cannot be opened.
		permission_denied,
		/// The buffer failed to handle the data
		buffer_is_null
	};
}