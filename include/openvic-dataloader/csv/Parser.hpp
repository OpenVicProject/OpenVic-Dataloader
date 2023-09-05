#pragma once

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/detail/BasicParser.hpp>

namespace ovdl::csv {
	enum class EncodingType {
		/// Support for Windows-1252 ANSI standard
		Windows1252,
		/// Support for UTF-8 Unicode standard
		Utf8
	};

	/// Parser for CSV files
	/// @tparam Encoding The type of encoding used for this parser
	template<EncodingType Encoding = EncodingType::Windows1252>
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

		/// Performs a CSV file parse over the buffer
		/// @return true Successfully parsed a CSV buffer
		/// @return false Failed to parse a CSV buffer
		/// @note Warnings still produce true
		bool parse_csv();

		/// Get a vector LineObjects
		/// @return const std::vector<csv::LineObject>&
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

	using Windows1252Parser = Parser<EncodingType::Windows1252>;
	using Utf8Parser = Parser<EncodingType::Utf8>;
}