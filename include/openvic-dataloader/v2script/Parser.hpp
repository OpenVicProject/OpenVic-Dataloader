#pragma once

#include <cstddef>
#include <cstdio>
#include <ostream>

namespace ovdl::v2script {
	class Parser {
	public:
		static Parser from_buffer(char8_t* data, std::size_t size);
		static Parser from_buffer(char8_t* start, char8_t* end);
		static Parser from_file(const char8_t* path);

		void set_error_log_to_stderr();
		void set_error_log_path(const char8_t* path);
		void set_error_log_to(std::basic_ostream<char8_t> stream);
		void set_error_log_to(std::FILE* file);

		bool parse();

		bool has_error();
		bool has_warning();

	private:
		Parser();
	};
}