#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <string_view>

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

		static Parser from_buffer(const char* data, std::size_t size);
		static Parser from_buffer(const char* start, const char* end);
		static Parser from_string(const std::string_view string);
		static Parser from_file(const char* path);
		static Parser from_file(const std::filesystem::path& path);

		constexpr Parser& load_from_buffer(const char* data, std::size_t size);
		constexpr Parser& load_from_buffer(const char* start, const char* end);
		constexpr Parser& load_from_string(const std::string_view string);
		constexpr Parser& load_from_file(const char* path);
		Parser& load_from_file(const std::filesystem::path& path);

		constexpr Parser& load_from_file(const detail::Has_c_str auto& path);

		bool simple_parse();
		bool event_parse();
		bool decision_parse();
		bool lua_defines_parse();

		const FileNode* get_file_node() const;

		void generate_node_location_map();

		const ast::Node::line_col get_node_begin(const ast::NodeCPtr node) const;
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