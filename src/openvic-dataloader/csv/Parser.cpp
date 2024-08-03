#include <iostream>
#include <optional>
#include <type_traits>
#include <vector>

#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/detail/Encoding.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include <dryad/node.hpp>

#include "CsvGrammar.hpp"
#include "CsvParseState.hpp"
#include "detail/NullBuff.hpp"
#include "detail/ParseHandler.hpp"

using namespace ovdl;
using namespace ovdl::csv;

///	ParseHandler ///

struct Parser::ParseHandler final : detail::BasicFileParseHandler<CsvParseState> {
	template<typename Node>
	std::optional<DiagnosticLogger::error_range> parse() {
		auto result = [&] {
			switch (parse_state().encoding()) {
				using enum detail::Encoding;
				case Ascii:
				case Utf8:
					return lexy::parse<Node>(buffer<lexy::utf8_char_encoding>(), parse_state(), parse_state().logger().error_callback());
				case Unknown:
				case Windows1251:
				case Windows1252:
					return lexy::parse<Node>(buffer<lexy::default_encoding>(), parse_state(), parse_state().logger().error_callback());
				default:
					ovdl::detail::unreachable();
			}
		}();
		if (!result) {
			return this->parse_state().logger().get_errors();
		}
		_lines = LEXY_MOV(result).value();
		return std::nullopt;
	}

	std::vector<csv::LineObject>& get_lines() {
		return _lines;
	}

	Parser::error_range get_errors() {
		return parse_state().logger().get_errors();
	}

private:
	std::vector<csv::LineObject> _lines;
};

///	ParserHandler ///

Parser::Parser()
	: _parse_handler(std::make_unique<ParseHandler>()) {
	set_error_log_to_null();
}

Parser::Parser(std::basic_ostream<char>& error_stream)
	: _parse_handler(std::make_unique<ParseHandler>()) {
	set_error_log_to(error_stream);
}

Parser::Parser(Parser&&) = default;
Parser& Parser::operator=(Parser&&) = default;
Parser::~Parser() = default;

Parser Parser::from_buffer(const char* data, std::size_t size, std::optional<detail::Encoding> encoding_fallback) {
	Parser result;
	return std::move(result.load_from_buffer(data, size, encoding_fallback));
}

Parser Parser::from_buffer(const char* start, const char* end, std::optional<detail::Encoding> encoding_fallback) {
	Parser result;
	return std::move(result.load_from_buffer(start, end, encoding_fallback));
}

Parser Parser::from_string(const std::string_view string, std::optional<detail::Encoding> encoding_fallback) {
	Parser result;
	return std::move(result.load_from_string(string, encoding_fallback));
}

Parser Parser::from_file(const char* path, std::optional<detail::Encoding> encoding_fallback) {
	Parser result;
	return std::move(result.load_from_file(path, encoding_fallback));
}

Parser Parser::from_file(const std::filesystem::path& path, std::optional<detail::Encoding> encoding_fallback) {
	Parser result;
	return std::move(result.load_from_file(path, encoding_fallback));
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
constexpr void Parser::_run_load_func(detail::LoadCallback<ParseHandler, Args...> auto func, Args... args) {
	_has_fatal_error = false;
	auto error = func(_parse_handler.get(), std::forward<Args>(args)...);
	auto error_message = _parse_handler->make_error_from(error);
	if (!error_message.empty()) {
		_has_error = true;
		_has_fatal_error = true;
		_parse_handler->parse_state().logger().template create_log<error::BufferError>(DiagnosticLogger::DiagnosticKind::error, fmt::runtime(error_message), _file_path);
	}
	if (has_error() && &_error_stream.get() != &detail::cnull) {
		print_errors_to(_error_stream.get());
	}
}

Parser& Parser::load_from_buffer(const char* data, std::size_t size, std::optional<detail::Encoding> encoding_fallback) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer_size), data, size, encoding_fallback);
	return *this;
}

Parser& Parser::load_from_buffer(const char* start, const char* end, std::optional<detail::Encoding> encoding_fallback) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer), start, end, encoding_fallback);
	return *this;
}

Parser& Parser::load_from_string(const std::string_view string, std::optional<detail::Encoding> encoding_fallback) {
	return load_from_buffer(string.data(), string.size(), encoding_fallback);
}

Parser& Parser::load_from_file(const char* path, std::optional<detail::Encoding> encoding_fallback) {
	set_file_path(path);
	// Type can be deduced??
	_run_load_func(std::mem_fn(&ParseHandler::load_file), get_file_path().data(), encoding_fallback);
	return *this;
}

Parser& Parser::load_from_file(const std::filesystem::path& path, std::optional<detail::Encoding> encoding_fallback) {
	return load_from_file(path.string().c_str(), encoding_fallback);
}

bool Parser::parse_csv(bool handle_strings) {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	std::optional<Parser::error_range> errors = [&] {
		if (handle_strings)
			return _parse_handler->template parse<csv::grammar::strings::SemiColonFile>();
		else
			return _parse_handler->template parse<csv::grammar::SemiColonFile>();
	}();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (errors && !errors->empty()) {
		_has_error = true;
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	return true;
}

const std::vector<csv::LineObject>& Parser::get_lines() const {
	return _parse_handler->get_lines();
}

typename Parser::error_range Parser::get_errors() const {
	return _parse_handler->get_errors();
}

std::string_view Parser::error(const ovdl::error::Error* error) const {
	return error->message(_parse_handler->parse_state().logger().symbol_interner());
}

const FilePosition Parser::get_error_position(const error::Error* error) const {
	if (!error || !error->is_linked_in_tree()) {
		return {};
	}
	auto err_location = _parse_handler->parse_state().logger().location_of(error);
	if (err_location.is_synthesized()) {
		return {};
	}

// TODO: Remove reinterpret_cast
// WARNING: This almost certainly breaks on utf16 and utf32 encodings, luckily we don't parse in that format
// This is purely to silence the node_location errors because char8_t is useless
#define REINTERPRET_IT(IT) reinterpret_cast<const std::decay_t<decltype(buffer)>::encoding::char_type*>((IT))

	return _parse_handler->parse_state().file().visit_buffer(
		[&](auto&& buffer) -> FilePosition {
			auto loc_begin = lexy::get_input_location(buffer, REINTERPRET_IT(err_location.begin()));
			FilePosition result { loc_begin.line_nr(), loc_begin.line_nr(), loc_begin.column_nr(), loc_begin.column_nr() };
			if (err_location.begin() < err_location.end()) {
				auto loc_end = lexy::get_input_location(buffer, REINTERPRET_IT(err_location.end()), loc_begin.anchor());
				result.end_line = loc_end.line_nr();
				result.end_column = loc_end.column_nr();
			}
			return result;
		});

#undef REINTERPRET_IT
}

void Parser::print_errors_to(std::basic_ostream<char>& stream) const {
	auto errors = get_errors();
	if (errors.empty()) return;
	for (const auto error : errors) {
		dryad::visit_tree(
			error,
			[&](const error::BufferError* buffer_error) {
				stream << this->error(buffer_error) << '\n';
			},
			[&](dryad::child_visitor<error::ErrorKind> visitor, const error::AnnotatedError* annotated_error) {
				stream << this->error(annotated_error) << '\n';
				auto annotations = annotated_error->annotations();
				for (auto annotation : annotations) {
					visitor(annotation);
				}
			},
			[&](const error::PrimaryAnnotation* primary) {
				stream << this->error(primary) << '\n';
			},
			[&](const error::SecondaryAnnotation* secondary) {
				stream << this->error(secondary) << '\n';
			});
	}
}