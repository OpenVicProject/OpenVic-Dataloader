#pragma once

#include <cstddef>
#include <filesystem>
#include <memory>
#include <ostream>
#include <string>
#include <string_view>

#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/Parser.hpp>
#include <openvic-dataloader/detail/utility/Concepts.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <dryad/node.hpp>

namespace ovdl::v2script {
	using FileTree = ast::FileTree;
	using FilePosition = ovdl::FilePosition;

	class Parser final : public detail::BasicParser {
	public:
		Parser();
		Parser(std::basic_ostream<char>& error_stream);

		static Parser from_buffer(const char* data, std::size_t size);
		static Parser from_buffer(const char* start, const char* end);
		static Parser from_string(const std::string_view string);
		static Parser from_file(const char* path);
		static Parser from_file(const std::filesystem::path& path);

		constexpr Parser& load_from_buffer(const char* data, std::size_t size);
		constexpr Parser& load_from_buffer(const char* start, const char* end);
		constexpr Parser& load_from_string(const std::string_view string);
		Parser& load_from_file(const char* path);
		Parser& load_from_file(const std::filesystem::path& path);

		constexpr Parser& load_from_file(const detail::HasCstr auto& path) {
			return load_from_file(path.c_str());
		}

		bool simple_parse();
		bool event_parse();
		bool decision_parse();
		bool lua_defines_parse();

		const FileTree* get_file_node() const;

		std::string_view value(const ovdl::v2script::ast::FlatValue* node) const;

		std::string make_native_string() const;
		std::string make_list_string() const;

		const FilePosition get_position(const ast::Node* node) const;

		using error_range = ovdl::detail::error_range<error::Root>;
		Parser::error_range get_errors() const;

		const FilePosition get_error_position(const error::Error* error) const;

		void print_errors_to(std::basic_ostream<char>& stream) const;

		Parser(Parser&&);
		Parser& operator=(Parser&&);

		~Parser();

	private:
		class ParseHandler;
		std::unique_ptr<ParseHandler> _parse_handler;

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<Parser::ParseHandler*, Args...> auto func, Args... args);
	};
}