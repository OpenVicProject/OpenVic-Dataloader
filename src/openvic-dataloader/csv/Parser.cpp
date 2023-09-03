#include <memory>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "csv/CsvGrammar.hpp"
#include "detail/BasicBufferHandler.hpp"
#include "detail/Errors.hpp"
#include "detail/LexyReportError.hpp"
#include "detail/OStreamOutputIterator.hpp"

using namespace ovdl;
using namespace ovdl::csv;

///	BufferHandler ///

class Parser::BufferHandler final : public detail::BasicBufferHandler<lexy::utf8_char_encoding> {
public:
	template<typename Node, typename ErrorCallback>
	std::optional<std::vector<ParseError>> parse(const ErrorCallback& callback) {
		auto result = lexy::parse<Node>(_buffer, callback);
		if (!result) {
			return result.errors();
		}
		_lines = std::move(result.value());
		return std::nullopt;
	}

	std::vector<csv::LineObject>& get_lines() {
		return _lines;
	}

private:
	std::vector<csv::LineObject> _lines;
};

/// BufferHandler ///

Parser::Parser()
	: _buffer_handler(std::make_unique<BufferHandler>()) {
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
/// @tparam Type
/// @tparam Args
/// @param func
/// @param args
///
template<typename... Args>
constexpr void Parser::_run_load_func(detail::LoadCallback<BufferHandler, Args...> auto func, Args... args) {
	_warnings.clear();
	_errors.clear();
	_has_fatal_error = false;
	if (auto error = func(_buffer_handler.get(), std::forward<Args>(args)...); error) {
		_has_fatal_error = error.value().type == ParseError::Type::Fatal;
		_errors.push_back(error.value());
		_error_stream.get() << "Error: " << _errors.back().message << '\n';
	}
}

constexpr Parser& Parser::load_from_buffer(const char* data, std::size_t size) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer_size), data, size);
	return *this;
}

constexpr Parser& Parser::load_from_buffer(const char* start, const char* end) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer), start, end);
	return *this;
}

constexpr Parser& Parser::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

constexpr Parser& Parser::load_from_file(const char* path) {
	_file_path = path;
	// Type can be deduced??
	_run_load_func(std::mem_fn(&BufferHandler::load_file), path);
	return *this;
}

Parser& Parser::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

constexpr Parser& Parser::load_from_file(const detail::Has_c_str auto& path) {
	return load_from_file(path.c_str());
}

bool Parser::parse_csv() {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	auto errors = _buffer_handler->parse<csv::grammar::SemiColonFile>(ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream }));
	if (errors) {
		_errors.reserve(errors->size());
		for (auto& err : errors.value()) {
			_has_fatal_error |= err.type == ParseError::Type::Fatal;
			_errors.push_back(err);
		}
		return false;
	}
	_lines = std::move(_buffer_handler->get_lines());
	return true;
}

const std::vector<csv::LineObject>& Parser::get_lines() const {
	return _lines;
}