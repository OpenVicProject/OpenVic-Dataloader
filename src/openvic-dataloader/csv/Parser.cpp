#include <vector>

#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/LexyReportError.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>
#include <openvic-dataloader/detail/utility/Utility.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "CsvGrammar.hpp"
#include "CsvParseState.hpp"
#include "detail/NullBuff.hpp"
#include "detail/ParseHandler.hpp"

using namespace ovdl;
using namespace ovdl::csv;

///	ParseHandler ///

template<EncodingType Encoding>
struct Parser<Encoding>::ParseHandler final : detail::BasicFileParseHandler<CsvParseState<Encoding>> {
	template<typename Node>
	std::optional<DiagnosticLogger::error_range> parse() {
		auto result = lexy::parse<Node>(this->buffer(), *this->_parse_state, this->_parse_state->logger().error_callback());
		if (!result) {
			return this->_parse_state->logger().get_errors();
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
	: _parse_handler(std::make_unique<ParseHandler>()) {
	set_error_log_to_null();
}

template<EncodingType Encoding>
Parser<Encoding>::Parser(std::basic_ostream<char>& error_stream)
	: _parse_handler(std::make_unique<ParseHandler>()) {
	set_error_log_to(error_stream);
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
constexpr void Parser<Encoding>::_run_load_func(detail::LoadCallback<ParseHandler, Args...> auto func, Args... args) {
	_has_fatal_error = false;
	auto error = func(_parse_handler.get(), std::forward<Args>(args)...);
	auto error_message = _parse_handler->make_error_from(error);
	if (!error_message.empty()) {
		_has_error = true;
		_has_fatal_error = true;
		_parse_handler->parse_state().logger().template create_log<error::BufferError>(DiagnosticLogger::DiagnosticKind::error, fmt::runtime(error_message));
	}
	if (has_error() && &_error_stream.get() != &detail::cnull) {
		print_errors_to(_error_stream.get());
	}
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_buffer(const char* data, std::size_t size) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer_size), data, size);
	return *this;
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_buffer(const char* start, const char* end) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer), start, end);
	return *this;
}

template<EncodingType Encoding>
constexpr Parser<Encoding>& Parser<Encoding>::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

template<EncodingType Encoding>
Parser<Encoding>& Parser<Encoding>::load_from_file(const char* path) {
	set_file_path(path);
	// Type can be deduced??
	_run_load_func(std::mem_fn(&ParseHandler::load_file), path);
	return *this;
}

template<EncodingType Encoding>
Parser<Encoding>& Parser<Encoding>::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

template<EncodingType Encoding>
bool Parser<Encoding>::parse_csv(bool handle_strings) {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	std::optional<Parser<Encoding>::error_range> errors;
	// auto report_error = ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream });
	if constexpr (Encoding == EncodingType::Windows1252) {
		if (handle_strings)
			errors = _parse_handler->template parse<csv::grammar::windows1252::strings::SemiColonFile>();
		else
			errors = _parse_handler->template parse<csv::grammar::windows1252::SemiColonFile>();
	} else {
		if (handle_strings)
			errors = _parse_handler->template parse<csv::grammar::utf8::strings::SemiColonFile>();
		else
			errors = _parse_handler->template parse<csv::grammar::utf8::SemiColonFile>();
	}
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!errors->empty()) {
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	_lines = std::move(_parse_handler->get_lines());
	return true;
}

template<EncodingType Encoding>
const std::vector<csv::LineObject>& Parser<Encoding>::get_lines() const {
	return _lines;
}

template<EncodingType Encoding>
typename Parser<Encoding>::error_range Parser<Encoding>::get_errors() const {
	return _parse_handler->parse_state().logger().get_errors();
}

template<EncodingType Encoding>
const FilePosition Parser<Encoding>::get_error_position(const error::Error* error) const {
	if (!error || !error->is_linked_in_tree()) {
		return {};
	}
	auto err_location = _parse_handler->parse_state().logger().location_of(error);
	if (err_location.is_synthesized()) {
		return {};
	}

	auto loc_begin = lexy::get_input_location(_parse_handler->buffer(), err_location.begin());
	FilePosition result { loc_begin.line_nr(), loc_begin.line_nr(), loc_begin.column_nr(), loc_begin.column_nr() };
	if (err_location.begin() < err_location.end()) {
		auto loc_end = lexy::get_input_location(_parse_handler->buffer(), err_location.end(), loc_begin.anchor());
		result.end_line = loc_end.line_nr();
		result.end_column = loc_end.column_nr();
	}
	return result;
}

template<EncodingType Encoding>
void Parser<Encoding>::print_errors_to(std::basic_ostream<char>& stream) const {
	auto errors = get_errors();
	if (errors.empty()) return;
	for (const auto error : errors) {
		dryad::visit_tree(
			error,
			[&](const error::BufferError* buffer_error) {
				stream << "buffer error: " << buffer_error->message() << '\n';
			},
			[&](const error::ParseError* parse_error) {
				auto position = get_error_position(parse_error);
				std::string pos_str = fmt::format(":{}:{}: ", position.start_line, position.start_column);
				stream << _file_path << pos_str << "parse error for '" << parse_error->production_name() << "': " << parse_error->message() << '\n';
			},
			[&](dryad::child_visitor<error::ErrorKind> visitor, const error::Semantic* semantic) {
				auto position = get_error_position(semantic);
				std::string pos_str = ": ";
				if (!position.is_empty()) {
					pos_str = fmt::format(":{}:{}: ", position.start_line, position.start_column);
				}
				stream << _file_path << pos_str << semantic->message() << '\n';
				auto annotations = semantic->annotations();
				for (auto annotation : annotations) {
					visitor(annotation);
				}
			},
			[&](const error::PrimaryAnnotation* primary) {
				stream << primary->message() << '\n';
			},
			[&](const error::SecondaryAnnotation* secondary) {
				stream << secondary->message() << '\n';
			});
	}
}

template class ovdl::csv::Parser<EncodingType::Windows1252>;
template class ovdl::csv::Parser<EncodingType::Utf8>;