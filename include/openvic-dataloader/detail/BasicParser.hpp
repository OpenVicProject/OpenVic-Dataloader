#pragma once

#include <string>
#include <string_view>
#include <vector>

#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/detail/Concepts.hpp>

namespace ovdl::detail {
	class BasicParser {
	public:
		BasicParser();

		void set_error_log_to_null();
		void set_error_log_to_stderr();
		void set_error_log_to_stdout();
		void set_error_log_to(std::basic_ostream<char>& stream);

		bool has_error() const;
		bool has_fatal_error() const;
		bool has_warning() const;

		const std::vector<ParseError>& get_errors() const;
		const std::vector<ParseWarning>& get_warnings() const;
		std::string_view get_file_path() const;

	protected:
		std::vector<ParseError> _errors;
		std::vector<ParseWarning> _warnings;

		std::reference_wrapper<std::ostream> _error_stream;
		std::string _file_path;
		bool _has_fatal_error = false;
	};
}