#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string>
#include <vector>

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace ovdl::v2script {

	using FileNode = ast::FileNode;

	class Parser {
	public:
		struct Error {
			const enum class Type : unsigned char {
				Recoverable,
				Fatal
			} type;
			const std::string message;
			const int error_value;
		};

		struct Warning {
			const std::string message;
			const int warning_value;
		};

		Parser();

		static Parser from_buffer(const char* data, std::size_t size);
		static Parser from_buffer(const char* start, const char* end);
		static Parser from_file(const char* path);

		Parser& load_from_buffer(const char* data, std::size_t size);
		Parser& load_from_buffer(const char* start, const char* end);
		Parser& load_from_file(const char* path);

		void set_error_log_to_null();
		void set_error_log_to_stderr();
		void set_error_log_to_stdout();
		void set_error_log_to(std::basic_ostream<char>& stream);

		bool simple_parse();

		bool has_error() const;
		bool has_fatal_error() const;
		bool has_warning() const;

		const std::vector<Error>& get_errors() const;
		const std::vector<Warning>& get_warnings() const;

		const FileNode* get_file_node() const;

		Parser(Parser&&);
		Parser& operator=(Parser&&);

		~Parser();

	private:
		std::vector<Error> _errors;
		std::vector<Warning> _warnings;

		class BufferHandler;
		friend class BufferHandler;
		std::unique_ptr<BufferHandler> _buffer_handler;
		std::unique_ptr<FileNode> _file_node;
		std::reference_wrapper<std::ostream> _error_stream;
		const char* _file_path;
		bool _has_fatal_error = false;

		template<typename... Args>
		inline void _run_load_func(std::optional<Error> (BufferHandler::*func)(Args...), Args... args);
	};
}