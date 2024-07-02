#include "openvic-dataloader/v2script/Parser.hpp"

#include <filesystem>
#include <iostream>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/Concepts.hpp>
#include <openvic-dataloader/detail/Encoding.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>
#include <openvic-dataloader/detail/Utility.hpp>
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

#include "DiagnosticLogger.hpp"
#include "ParseState.hpp"
#include "detail/NullBuff.hpp"
#include "detail/ParseHandler.hpp"
#include "detail/Warnings.hpp"
#include "v2script/DecisionGrammar.hpp"
#include "v2script/EventGrammar.hpp"
#include "v2script/LuaDefinesGrammar.hpp"
#include "v2script/SimpleGrammar.hpp"

using namespace ovdl;
using namespace ovdl::v2script;

///	ParseHandler ///

struct Parser::ParseHandler final : detail::BasicStateParseHandler<v2script::ast::ParseState> {
	template<typename Node>
	std::optional<DiagnosticLogger::error_range> parse() {
		if (parse_state().encoding() == ovdl::detail::Encoding::Utf8) {
			parse_state().logger().warning(warnings::make_utf8_warning(path()));
		}

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
			return parse_state().logger().get_errors();
		}
		parse_state().ast().set_root(result.value());
		return std::nullopt;
	}

	ast::FileTree* root() {
		return parse_state().ast().root();
	}

	Parser::error_range get_errors() {
		using iterator = typename decltype(std::declval<const error::Root*>()->children())::iterator;
		if (!is_valid())
			return dryad::make_node_range<error::Error>(iterator::from_ptr(nullptr), iterator::from_ptr(nullptr));
		return parse_state().logger().get_errors();
	}
};

///	ParseHandler ///

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
constexpr void Parser::_run_load_func(detail::LoadCallback<Parser::ParseHandler*, Args...> auto func, Args... args) {
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

	std::optional<DiagnosticLogger::error_range> errors = _parse_handler->parse<grammar::File>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_error = true;
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

	std::optional<DiagnosticLogger::error_range> errors = _parse_handler->parse<grammar::EventFile>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_error = true;
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

	std::optional<DiagnosticLogger::error_range> errors = _parse_handler->parse<grammar::DecisionFile>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_error = true;
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

	std::optional<DiagnosticLogger::error_range> errors = _parse_handler->parse<lua::grammar::File>();
	_has_error = _parse_handler->parse_state().logger().errored();
	_has_warning = _parse_handler->parse_state().logger().warned();
	if (!_parse_handler->root()) {
		_has_error = true;
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

std::string_view Parser::value(const ovdl::v2script::ast::FlatValue* node) const {
	return node->value().view();
}

std::string Parser::make_native_string() const {
	return _parse_handler->parse_state().ast().make_native_visualizer();
}

std::string Parser::make_list_string() const {
	return _parse_handler->parse_state().ast().make_list_visualizer();
}

// TODO: Remove reinterpret_cast
// WARNING: This almost certainly breaks on utf16 and utf32 encodings, luckily we don't parse in that format
// This is purely to silence the node_location errors because char8_t is useless
#define REINTERPRET_IT(IT) reinterpret_cast<const std::decay_t<decltype(buffer)>::encoding::char_type*>((IT))

const FilePosition Parser::get_position(const ast::Node* node) const {
	if (!node || !node->is_linked_in_tree()) {
		return {};
	}

	NodeLocation node_location;

	node_location = _parse_handler->parse_state().ast().location_of(node);

	if (node_location.is_synthesized()) {
		return FilePosition {};
	}

	return _parse_handler->parse_state().ast().file().visit_buffer(
		[&](auto&& buffer) -> FilePosition {
			auto loc_begin = lexy::get_input_location(buffer, REINTERPRET_IT(node_location.begin()));
			FilePosition result { loc_begin.line_nr(), loc_begin.line_nr(), loc_begin.column_nr(), loc_begin.column_nr() };
			if (node_location.begin() < node_location.end()) {
				auto loc_end = lexy::get_input_location(buffer, REINTERPRET_IT(node_location.end()), loc_begin.anchor());
				result.end_line = loc_end.line_nr();
				result.end_column = loc_end.column_nr();
			}
			return result;
		});
}

Parser::error_range Parser::get_errors() const {
	return _parse_handler->get_errors();
}

const FilePosition Parser::get_error_position(const error::Error* error) const {
	if (!error || !error->is_linked_in_tree()) {
		return {};
	}

	auto err_location = _parse_handler->parse_state().logger().location_of(error);
	if (err_location.is_synthesized()) {
		return FilePosition {};
	}

	return _parse_handler->parse_state().ast().file().visit_buffer(
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
}

#undef REINTERPRET_IT

void Parser::print_errors_to(std::basic_ostream<char>& stream) const {
	auto errors = get_errors();
	if (errors.empty()) return;
	for (const auto error : errors) {
		dryad::visit_tree(
			error,
			[&](const error::BufferError* buffer_error) {
				stream << "buffer error: " << buffer_error->message() << '\n';
			},
			[&](dryad::child_visitor<error::ErrorKind> visitor, const error::AnnotatedError* annotated_error) {
				stream << annotated_error->message() << '\n';
				auto annotations = annotated_error->annotations();
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