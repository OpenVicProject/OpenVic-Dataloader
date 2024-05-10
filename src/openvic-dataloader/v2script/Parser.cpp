#include "openvic-dataloader/v2script/Parser.hpp"

#include <iostream>
#include <optional>
#include <string>
#include <utility>

#include <openvic-dataloader/DiagnosticLogger.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/detail/LexyReportError.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>
#include <openvic-dataloader/detail/utility/Concepts.hpp>
#include <openvic-dataloader/detail/utility/Utility.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>
#include <lexy/input_location.hpp>
#include <lexy/lexeme.hpp>
#include <lexy/visualize.hpp>

#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <fmt/core.h>

#include "openvic-dataloader/Error.hpp"

#include "detail/DetectUtf8.hpp"
#include "detail/NullBuff.hpp"
#include "detail/ParseHandler.hpp"
#include "detail/Warnings.hpp"
#include "v2script/DecisionGrammar.hpp"
#include "v2script/EventGrammar.hpp"
#include "v2script/LuaDefinesGrammar.hpp"
#include "v2script/SimpleGrammar.hpp"

using namespace ovdl;
using namespace ovdl::v2script;

///	BufferHandler ///

struct Parser::ParseHandler final : detail::BasicStateParseHandler<v2script::ast::ParseState> {
	constexpr bool is_exclusive_utf8() const {
		return detail::is_utf8_no_ascii(buffer());
	}

	template<typename Node>
	std::optional<DiagnosticLogger::error_range> parse() {
		auto result = lexy::parse<Node>(buffer(), *_parse_state, _parse_state->logger().error_callback());
		if (!result) {
			return _parse_state->logger().get_errors();
		}
		_parse_state->ast().set_root(result.value());
		return std::nullopt;
	}

	ast::FileTree* root() {
		return _parse_state->ast().root();
	}
};

/// BufferHandler ///

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
constexpr void Parser::_run_load_func(detail::LoadCallback<Parser::ParseHandler*, Args...> auto func, Args... args) {
	_has_fatal_error = false;
	auto error = func(_parse_handler.get(), std::forward<Args>(args)...);
	auto error_message = _parse_handler->make_error_from(error);
	if (!error_message.empty()) {
		_has_error = true;
		_has_fatal_error = true;
		_parse_handler->parse_state().logger().create_log<error::BufferError>(DiagnosticLogger::DiagnosticKind::error, fmt::runtime(error_message));
	}
	if (has_error() && &_error_stream.get() != &detail::cnull) {
		print_errors_to(_error_stream.get());
	}
}

constexpr Parser& Parser::load_from_buffer(const char* data, std::size_t size) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer_size), data, size);
	return *this;
}

constexpr Parser& Parser::load_from_buffer(const char* start, const char* end) {
	// Type can't be deduced?
	_run_load_func(std::mem_fn(&ParseHandler::load_buffer), start, end);
	return *this;
}

constexpr Parser& Parser::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

constexpr Parser& Parser::load_from_file(const char* path) {
	_file_path = path;
	// Type can be deduced??
	_run_load_func(std::mem_fn(&ParseHandler::load_file), path);
	return *this;
}

Parser& Parser::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

/* REQUIREMENTS:
 * DAT-23
 * DAT-26
 * DAT-28
 * DAT-29
 */
bool Parser::simple_parse() {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	if (_parse_handler->is_exclusive_utf8()) {
		_parse_handler->parse_state().logger().warning(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _parse_handler->parse<grammar::File<grammar::NoStringEscapeOption>>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	return true;
}

bool Parser::event_parse() {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	if (_parse_handler->is_exclusive_utf8()) {
		_parse_handler->parse_state().logger().warning(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _parse_handler->parse<grammar::EventFile>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	return true;
}

bool Parser::decision_parse() {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	if (_parse_handler->is_exclusive_utf8()) {
		_parse_handler->parse_state().logger().warning(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _parse_handler->parse<grammar::DecisionFile>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	return true;
}

bool Parser::lua_defines_parse() {
	if (!_parse_handler->is_valid()) {
		return false;
	}

	if (_parse_handler->is_exclusive_utf8()) {
		_parse_handler->parse_state().logger().warning(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _parse_handler->parse<lua::grammar::File<>>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_fatal_error = true;
		if (&_error_stream.get() != &detail::cnull) {
			print_errors_to(_error_stream);
		}
		return false;
	}
	return true;
}

const FileTree* Parser::get_file_node() const {
	return _parse_handler->root();
}

std::string_view Parser::value(const ovdl::v2script::ast::FlatValue& node) const {
	return node.value(_parse_handler->parse_state().ast().symbol_interner());
}

std::string Parser::make_native_string() const {
	return _parse_handler->parse_state().ast().make_native_visualizer();
}

std::string Parser::make_list_string() const {
	return _parse_handler->parse_state().ast().make_list_visualizer();
}

const FilePosition Parser::get_position(const ast::Node* node) const {
	if (!node || !node->is_linked_in_tree()) {
		return {};
	}
	auto node_location = _parse_handler->parse_state().ast().location_of(node);
	if (node_location.is_synthesized()) {
		return {};
	}

	auto loc_begin = lexy::get_input_location(_parse_handler->buffer(), node_location.begin());
	FilePosition result { loc_begin.line_nr(), loc_begin.line_nr(), loc_begin.column_nr(), loc_begin.column_nr() };
	if (node_location.begin() < node_location.end()) {
		auto loc_end = lexy::get_input_location(_parse_handler->buffer(), node_location.end(), loc_begin.anchor());
		result.end_line = loc_end.line_nr();
		result.end_column = loc_end.column_nr();
	}
	return result;
}

Parser::error_range Parser::get_errors() const {
	return _parse_handler->parse_state().logger().get_errors();
}

const FilePosition Parser::get_error_position(const error::Error* error) const {
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

void Parser::print_errors_to(std::basic_ostream<char>& stream) const {
	auto errors = get_errors();
	if (errors.empty()) return;
	for (const auto error : errors) {
		dryad::visit_tree(
			error,
			[&](const error::BufferError* buffer_error) {
				stream << buffer_error->message() << '\n';
			},
			[&](const error::ParseError* parse_error) {
				stream << parse_error->message() << '\n';
			},
			[&](dryad::child_visitor<error::ErrorKind> visitor, const error::Semantic* semantic) {
				stream << semantic->message() << '\n';
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