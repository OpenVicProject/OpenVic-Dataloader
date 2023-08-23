#include "openvic-dataloader/v2script/Parser.hpp"

#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "SimpleGrammar.hpp"
#include "detail/DetectUtf8.hpp"
#include "detail/Errors.hpp"
#include "detail/LexyReportError.hpp"
#include "detail/NullBuff.hpp"
#include "detail/Warnings.hpp"
#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>
#include <lexy/lexeme.hpp>
#include <lexy/visualize.hpp>
#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

using namespace ovdl;
using namespace ovdl::v2script;

///	BufferHandler ///

class Parser::BufferHandler {
public:
	bool is_valid() const {
		return _buffer.size() != 0;
	}

	std::optional<ParseError> load_buffer(const char* data, std::size_t size) {
		_buffer = lexy::buffer(data, size);
		return std::nullopt;
	}

	std::optional<ParseError> load_buffer(const char* start, const char* end) {
		_buffer = lexy::buffer(start, end);
		return std::nullopt;
	}

	std::optional<ParseError> load_file(const char* path) {
		auto file = lexy::read_file(path);
		if (!file) {
			return errors::make_no_file_error(path);
		}

		_buffer = file.buffer();
		return std::nullopt;
	}

	constexpr bool is_exclusive_utf8() const {
		return detail::is_utf8_no_ascii(_buffer);
	}

	template<typename Node, typename ErrorCallback>
	std::optional<std::vector<ParseError>> parse(const ErrorCallback& callback) {
		auto result = lexy::parse<Node>(_buffer, callback);
		if (!result) {
			return result.errors();
		}
		// This is mighty frustrating
		_root = std::unique_ptr<ast::Node>(result.value());
		return std::nullopt;
	}

	std::unique_ptr<ast::Node>& get_root() {
		return _root;
	}

private:
	lexy::buffer<lexy::default_encoding> _buffer;
	std::unique_ptr<ast::Node> _root;
};

/// BufferHandler ///

Parser::Parser()
	: _buffer_handler(std::make_unique<BufferHandler>()),
	  _error_stream(detail::cnull) {
	set_error_log_to_stderr();
}

Parser::Parser(Parser&&) = default;
Parser& Parser::operator=(Parser&&) = default;
Parser::~Parser() = default;

Parser Parser::from_buffer(const char* data, std::size_t size) {
	Parser result;
	return std::move(result.load_from_buffer(data, size));
}

Parser Parser::from_buffer(const char* start, const char* end) {
	Parser result;
	return std::move(result.load_from_buffer(start, end));
}

Parser Parser::from_string(const std::string_view string) {
	Parser result;
	return std::move(result.load_from_string(string));
}

Parser Parser::from_file(const char* path) {
	Parser result;
	return std::move(result.load_from_file(path));
}

Parser Parser::from_file(const std::filesystem::path& path) {
	Parser result;
	return std::move(result.load_from_file(path));
}

///
/// @brief Executes a function on _buffer_handler that is expected to load a buffer
///
/// Expected Use:
/// @code {.cpp}
///	_run_load_func(&BufferHandler::<load_function>, <arguments>);
/// @endcode
///
/// @tparam Args
/// @param func
/// @param args
///
template<typename... Args>
inline void Parser::_run_load_func(std::optional<ParseError> (BufferHandler::*func)(Args...), Args... args) {
	_warnings.clear();
	_errors.clear();
	_has_fatal_error = false;
	if (auto error = (_buffer_handler.get()->*func)(args...); error) {
		_has_fatal_error = error.value().type == ParseError::Type::Fatal;
		_errors.push_back(error.value());
		_error_stream.get() << "Error: " << _errors.back().message << '\n';
	}
}

Parser& Parser::load_from_buffer(const char* data, std::size_t size) {
	_run_load_func(&BufferHandler::load_buffer, data, size);
	return *this;
}

Parser& Parser::load_from_buffer(const char* start, const char* end) {
	_run_load_func(&BufferHandler::load_buffer, start, end);
	return *this;
}

Parser& Parser::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

Parser& Parser::load_from_file(const char* path) {
	_file_path = path;
	_run_load_func(&BufferHandler::load_file, path);
	return *this;
}

Parser& Parser::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

void Parser::set_error_log_to_null() {
	set_error_log_to(detail::cnull);
}

void Parser::set_error_log_to_stderr() {
	set_error_log_to(std::cerr);
}

void Parser::set_error_log_to_stdout() {
	set_error_log_to(std::cout);
}

void Parser::set_error_log_to(std::basic_ostream<char>& stream) {
	_error_stream = stream;
}

bool Parser::simple_parse() {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	struct ostream_output_iterator {
		std::reference_wrapper<std::ostream> _stream;

		auto operator*() const noexcept {
			return *this;
		}
		auto operator++(int) const noexcept {
			return *this;
		}

		ostream_output_iterator& operator=(char c) {
			_stream.get().put(c);
			return *this;
		}
	};

	if (_buffer_handler->is_exclusive_utf8()) {
		_warnings.push_back(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _buffer_handler->parse<grammar::File>(ovdl::detail::ReporError.path(_file_path).to(ostream_output_iterator { _error_stream }));
	if (errors) {
		_errors.reserve(errors->size());
		for (auto& err : errors.value()) {
			_has_fatal_error |= err.type == ParseError::Type::Fatal;
			_errors.push_back(err);
		}
		return false;
	}
	_file_node.reset(static_cast<ast::FileNode*>(_buffer_handler->get_root().release()));
	return true;
}

bool Parser::has_error() const {
	return !_errors.empty();
}

bool Parser::has_fatal_error() const {
	return _has_fatal_error;
}

bool Parser::has_warning() const {
	return !_warnings.empty();
}

const std::vector<ovdl::ParseError>& Parser::get_errors() const {
	return _errors;
}

const std::vector<ovdl::ParseWarning>& Parser::get_warnings() const {
	return _warnings;
}

const FileNode* Parser::get_file_node() const {
	return _file_node.get();
}