#include <memory>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/ClassExport.hpp>

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

template<EncodingType>
struct LexyEncodingFrom {
};

template<>
struct LexyEncodingFrom<EncodingType::Windows1252> {
	using encoding = lexy::default_encoding;
};

template<>
struct LexyEncodingFrom<EncodingType::Utf8> {
	using encoding = lexy::utf8_char_encoding;
};

template<EncodingType Encoding>
class Parser<Encoding>::BufferHandler final : public detail::BasicBufferHandler<typename LexyEncodingFrom<Encoding>::encoding> {
public:
	template<typename Node, typename ParseState, typename ErrorCallback>
	std::optional<std::vector<ParseError>> parse(const ParseState& state, const ErrorCallback& callback) {
		auto result = lexy::parse<Node>(this->_buffer, state, callback);
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

template<EncodingType Encoding>
Parser<Encoding>::Parser()
	: _buffer_handler(std::make_unique<BufferHandler>()) {
	set_error_log_to_stderr();
}

template<EncodingType Encoding>
Parser<Encoding>::Parser(Parser&&) = default;
template<EncodingType Encoding>
Parser<Encoding>& Parser<Encoding>::operator=(Parser&&) = default;
template<EncodingType Encoding>
Parser<Encoding>::~Parser() = default;

template<EncodingType Encoding>
Parser<Encoding> Parser<Encoding>::from_buffer(const char* data, std::size_t size) {
	Parser result;
	return std::move(result.load_from_buffer(data, size));
}

template<EncodingType Encoding>
Parser<Encoding> Parser<Encoding>::from_buffer(const char* start, const char* end) {
	Parser result;
	return std::move(result.load_from_buffer(start, end));
}

template<EncodingType Encoding>
Parser<Encoding> Parser<Encoding>::from_string(const std::string_view string) {
	Parser result;
	return std::move(result.load_from_string(string));
}

template<EncodingType Encoding>
Parser<Encoding> Parser<Encoding>::from_file(const char* path) {
	Parser result;
	return std::move(result.load_from_file(path));
}

template<EncodingType Encoding>
Parser<Encoding> Parser<Encoding>::from_file(const std::filesystem::path& path) {
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
template<EncodingType Encoding>
template<typename... Args>
constexpr void Parser<Encoding>::_run_load_func(detail::LoadCallback<BufferHandler, Args...> auto func, Args... args) {
	_warnings.clear();
	_errors.clear();
	_has_fatal_error = false;
	if (auto error = func(_buffer_handler.get(), std::forward<Args>(args)...); error) {
		_has_fatal_error = error.value().type == ParseError::Type::Fatal;
		_errors.push_back(error.value());
		_error_stream.get() << "Error: " << _errors.back().message << '\n';
	}
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_buffer(const char* data, std::size_t size) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer_size), data, size);
	return *this;
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_buffer(const char* start, const char* end) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer), start, end);
	return *this;
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_file(const char* path) {
	_file_path = path;
	// Type can be deduced??
	_run_load_func(std::mem_fn(&BufferHandler::load_file), path);
	return *this;
}

template<EncodingType Encoding>
Parser<Encoding>& Parser<Encoding>::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_file(const detail::Has_c_str auto& path) {
	return load_from_file(path.c_str());
}

template<EncodingType Encoding>
bool Parser<Encoding>::parse_csv(bool handle_strings) {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	std::optional<std::vector<ParseError>> errors;
	auto report_error = ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream });
	if constexpr (Encoding == EncodingType::Windows1252) {
		if (handle_strings)
			errors = _buffer_handler->template parse<csv::grammar::windows1252::strings::SemiColonFile>(_parser_state, report_error);
		else
			errors = _buffer_handler->template parse<csv::grammar::windows1252::SemiColonFile>(_parser_state, report_error);
	} else {
		if (handle_strings)
			errors = _buffer_handler->template parse<csv::grammar::utf8::strings::SemiColonFile>(_parser_state, report_error);
		else
			errors = _buffer_handler->template parse<csv::grammar::utf8::SemiColonFile>(_parser_state, report_error);
	}
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

template<EncodingType Encoding>
const std::vector<csv::LineObject>& Parser<Encoding>::get_lines() const {
	return _lines;
}

template class ovdl::csv::Parser<EncodingType::Windows1252>;
template class ovdl::csv::Parser<EncodingType::Utf8>;