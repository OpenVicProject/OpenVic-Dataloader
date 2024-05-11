#pragma once

#include <filesystem>

#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/Parser.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/detail/utility/Concepts.hpp>
#include <openvic-dataloader/detail/utility/ErrorRange.hpp>

#include <dryad/node.hpp>

namespace ovdl::csv {
	enum class EncodingType {
		Windows1252,
		Utf8
	};

	template<EncodingType Encoding = EncodingType::Windows1252>
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

		bool parse_csv(bool handle_strings = false);

		const std::vector<csv::LineObject>& get_lines() const;

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
		std::vector<csv::LineObject> _lines;

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<ParseHandler, Args...> auto func, Args... args);
	};

	using Windows1252Parser = Parser<EncodingType::Windows1252>;
	using Utf8Parser = Parser<EncodingType::Utf8>;
}