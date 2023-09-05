#pragma once

#include <cstddef>
#include <filesystem>
#include <functional>
#include <memory>
#include <optional>
#include <ostream>
#include <string_view>
#include <vector>

#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/detail/BasicParser.hpp>
#include <openvic-dataloader/detail/Concepts.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

namespace ovdl::v2script {

	using FileNode = ast::FileNode;

	class Parser final : public detail::BasicParser {
	public:
		Parser();

		/// Makes a parse buffer from data to data+size
		/// @param data Initial character buffer memory position
		/// @param size End distance from data for buffer
		/// @return Parser that calls load_from_buffer(const char* data, std::size_t size)
		/// @sa load_from_buffer(const char* data, std::size_t size)
		static Parser from_buffer(const char* data, std::size_t size);
		/// Makes a parse buffer from data to end
		/// @param start Initial character buffer memory position
		/// @param end End character buffer memory position
		/// @return Parser that calls load_from_buffer(const char* start, const char* end)
		/// @sa load_from_buffer(const char* data, const char* end)
		static Parser from_buffer(const char* start, const char* end);
		/// Makes a parse buffer from a string_view
		/// @param string string_view to create a buffer from
		/// @return Parser that calls load_from_string(const std::string_view string)
		/// @sa load_from_string(const std::string_view string)
		static Parser from_string(const std::string_view string);
		/// Makes a parse buffer based on the file path
		/// @param path The file path to supply for the buffer
		/// @return Parser that calls load_from_file(const char* path)
		/// @sa load_from_file(const char* path)
		static Parser from_file(const char* path);
		/// Makes a parse buffer based on a filesystem::path
		/// @param path The file path to supply for the buffer
		/// @return Parser that calls load_from_file(const std::filesystem::path& path)
		/// @sa load_from_file(const std::filesystem::path& path)
		static Parser from_file(const std::filesystem::path& path);
		/// Makes a parse buffer based on any type that has a `const char* c_str()` function
		/// @param path The file path to supply for the buffer
		/// @return Parser that calls from_file(path.c_str())
		/// @sa from_file(const char* path)
		static Parser from_file(const detail::Has_c_str auto& path) {
			return from_file(path.c_str());
		}

		/// Loads a parse buffer from data to data+size
		/// @param data Initial character buffer memory position
		/// @param size End distance from data for buffer
		/// @return Parser& *this
		constexpr Parser& load_from_buffer(const char* data, std::size_t size);
		/// Loads a parse buffer from data to end
		/// @param start Initial character buffer memory position
		/// @param end End character buffer memory position
		/// @return Parser& *this
		constexpr Parser& load_from_buffer(const char* start, const char* end);
		/// Loads a parse buffer from a string_view
		/// @param string string_view to create a buffer from
		/// @return Parser& *this
		constexpr Parser& load_from_string(const std::string_view string);
		/// Loads a parse buffer based on the file path
		/// @param path The file path to supply for the buffer
		/// @return Parser& *this
		constexpr Parser& load_from_file(const char* path);
		/// Loads a parse buffer based on a filesystem::path
		/// @param path The file path to supply for the buffer
		/// @return Parser& *this
		Parser& load_from_file(const std::filesystem::path& path);

		/// Loads a parse buffer based on any type that has a `const char* c_str()` function
		/// @param path The file path to supply for the buffer
		/// @return Parser& *this
		constexpr Parser& load_from_file(const detail::Has_c_str auto& path) {
			return load_from_file(path.c_str());
		}

		/// Performs a less interpretive file parse over the buffer
		/// @return true Successfully parsed a simple text buffer
		/// @return false Failed to parse a simple text buffer
		/// @note Warnings still produce true
		/// @sa src/openvic-dataloader/v2script/SimpleGrammar.hpp
		bool simple_parse();
		/// Performs an event file parse over the buffer
		/// @return true Successfully parsed an event file
		/// @return false Failed to parse an event file
		/// @note Warnings still produce true
		/// @sa src/openvic-dataloader/v2script/EventGrammar.hpp
		bool event_parse();
		/// Performs a decision file parse over the buffer
		/// @return true Successfully parsed a decision file
		/// @return false Failed to parse an decision file
		/// @note Warnings still produce true
		/// @sa src/openvic-dataloader/v2script/DecisionGrammar.hpp
		bool decision_parse();

		/// @brief Gets the top-level FileNode pointer that was parsed
		/// @return const FileNode*
		const FileNode* get_file_node() const;

		/// Generates a map of line/column locations based on the buffer
		/// @note Call after parse, this relies on parsed nodes
		/// @note Currently ignores event and decision parsing
		void generate_node_location_map();

		/// Gets the node's begin line and column
		/// @param node Node pointer to get the beginning line and column of
		/// @return const ast::Node::line_col
		const ast::Node::line_col get_node_begin(const ast::NodeCPtr node) const;
		/// Gets the node's end line and column
		///
		/// If a node does not specify a seperate end, same as get_node_begin
		/// @param node Node pointer to get the ending line and column of
		/// @return const ast::Node::line_col
		const ast::Node::line_col get_node_end(const ast::NodeCPtr node) const;

		Parser(Parser&&);
		Parser& operator=(Parser&&);

		~Parser();

	private:
		friend class ::ovdl::v2script::ast::Node;
		class BufferHandler;
		std::unique_ptr<BufferHandler> _buffer_handler;
		std::unique_ptr<FileNode> _file_node;

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<BufferHandler, Args...> auto func, Args... args);
	};
}