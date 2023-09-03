#pragma once

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/detail/BasicParser.hpp>

namespace ovdl::csv {
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

		bool parse_csv();

		const std::vector<csv::LineObject>& get_lines() const;

		Parser(Parser&&);
		Parser& operator=(Parser&&);

		~Parser();

	private:
		class BufferHandler;
		std::unique_ptr<BufferHandler> _buffer_handler;
		std::vector<csv::LineObject> _lines;

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<BufferHandler, Args...> auto func, Args... args);
	};
}