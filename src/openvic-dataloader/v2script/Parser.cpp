#include "openvic-dataloader/v2script/Parser.hpp"

#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/detail/Concepts.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/NodeLocationMap.hpp>

#include <lexy/action/parse.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>
#include <lexy/lexeme.hpp>
#include <lexy/visualize.hpp>

#include "SimpleGrammar.hpp"
#include "detail/BasicBufferHandler.hpp"
#include "detail/DetectUtf8.hpp"
#include "detail/Errors.hpp"
#include "detail/LexyReportError.hpp"
#include "detail/NullBuff.hpp"
#include "detail/OStreamOutputIterator.hpp"
#include "detail/Warnings.hpp"
#include "v2script/DecisionGrammar.hpp"
#include "v2script/EventGrammar.hpp"

using namespace ovdl;
using namespace ovdl::v2script;

///	BufferHandler ///

class Parser::BufferHandler final : public detail::BasicBufferHandler<> {
public:
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

	ast::NodeLocationMap<decltype(_buffer)>& get_location_map() {
		return _location_map;
	}

private:
	friend class ::ovdl::v2script::ast::Node;
	std::unique_ptr<ast::Node> _root;
	ast::NodeLocationMap<decltype(_buffer)> _location_map;
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
///	_run_load_func(std::mem_fn(&BufferHandler::<load_function>), <arguments>);
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
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer_size), data, size);
	return *this;
}

constexpr Parser& Parser::load_from_buffer(const char* start, const char* end) {
	_run_load_func(std::mem_fn(&BufferHandler::load_buffer), start, end);
	return *this;
}

constexpr Parser& Parser::load_from_string(const std::string_view string) {
	return load_from_buffer(string.data(), string.size());
}

constexpr Parser& Parser::load_from_file(const char* path) {
	_file_path = path;
	_run_load_func(std::mem_fn(&BufferHandler::load_file), path);
	return *this;
}

Parser& Parser::load_from_file(const std::filesystem::path& path) {
	return load_from_file(path.string().c_str());
}

bool Parser::simple_parse() {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	if (_buffer_handler->is_exclusive_utf8()) {
		_warnings.push_back(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _buffer_handler->parse<grammar::File>(ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream }));
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

bool Parser::event_parse() {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	if (_buffer_handler->is_exclusive_utf8()) {
		_warnings.push_back(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _buffer_handler->parse<grammar::EventFile>(ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream }));
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

bool Parser::decision_parse() {
	if (!_buffer_handler->is_valid()) {
		return false;
	}

	if (_buffer_handler->is_exclusive_utf8()) {
		_warnings.push_back(warnings::make_utf8_warning(_file_path));
	}

	auto errors = _buffer_handler->parse<grammar::DecisionFile>(ovdl::detail::ReporError.path(_file_path).to(detail::OStreamOutputIterator { _error_stream }));
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

const FileNode* Parser::get_file_node() const {
	return _file_node.get();
}

void Parser::generate_node_location_map() {
	_buffer_handler->get_location_map().clear();
	_buffer_handler->get_location_map().generate_location_map(_buffer_handler->get_buffer(), get_file_node());
}

const ast::Node::line_col Parser::get_node_begin(const ast::NodeCPtr node) const {
	if (!node) return { 0, 0 };
	return node->get_begin_line_col(*this);
}

const ast::Node::line_col Parser::get_node_end(const ast::NodeCPtr node) const {
	if (!node) return { 0, 0 };
	return node->get_end_line_col(*this);
}

const ast::Node::line_col ast::Node::get_begin_line_col(const Parser& parser) const {
	if (!parser._buffer_handler->is_valid() || parser._buffer_handler->_location_map.empty()) return {};
	line_col result;
	auto [itr, range_end] = parser._buffer_handler->_location_map.equal_range(this);
	if (itr != range_end) {
		result.line = itr->second.line_nr();
		result.column = itr->second.column_nr();
	}
	// Standard doesn't really guarantee the direction of the range's sequence, but only GCC goes backwards
	// TODO: DON'T USE STANDARD UNORDERED_MULTIMAP
#if defined(__GNUC__) && !defined(__clang__)
	itr++;
	if (itr != range_end) {
		result.line = itr->second.line_nr();
		result.column = itr->second.column_nr();
	}
#endif
	return result;
}

const ast::Node::line_col ast::Node::get_end_line_col(const Parser& parser) const {
	if (!parser._buffer_handler->is_valid() || parser._buffer_handler->_location_map.empty()) return {};
	line_col result;
	auto [itr, range_end] = parser._buffer_handler->_location_map.equal_range(this);
	if (itr != range_end) {
		result.line = itr->second.line_nr();
		result.column = itr->second.column_nr();
	}
	// Standard doesn't really guarantee the direction of the range's sequence, but only GCC goes backwards
	// TODO: DON'T USE STANDARD UNORDERED_MULTIMAP
#if defined(__GNUC__) && !defined(__clang__)
	return result;
#endif
	itr++;
	if (itr != range_end) {
		result.line = itr->second.line_nr();
		result.column = itr->second.column_nr();
	}
	return result;
}