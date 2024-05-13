#pragma once

#include <string>
#include <string_view>

#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>

namespace ovdl::detail {
	struct BasicParser {
		BasicParser();

		void set_error_log_to_null();
		void set_error_log_to_stderr();
		void set_error_log_to_stdout();
		void set_error_log_to(std::basic_ostream<char>& stream);

		bool has_error() const;
		bool has_fatal_error() const;
		bool has_warning() const;

		std::string_view get_file_path() const;

	protected:
		void set_file_path(std::string_view path);

		std::reference_wrapper<std::ostream> _error_stream;
		std::string _file_path;
		bool _has_fatal_error = false;
		bool _has_error = false;
		bool _has_warning = false;
	};
}