#pragma once

#include <functional>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/detail/BasicParser.hpp>

namespace ovdl::csv {
	enum class EncodingType {
		Windows1252,
		Utf8
	};

	struct string_hash {
		using is_transparent = void;
		[[nodiscard]] size_t operator()(const char* txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] size_t operator()(std::string_view txt) const {
			return std::hash<std::string_view> {}(txt);
		}
		[[nodiscard]] size_t operator()(std::string& txt) const {
			return std::hash<std::string> {}(txt);
		}
	};

	template<EncodingType Encoding = EncodingType::Windows1252>
	class Parser final : public detail::BasicParser {
	public:
		struct State {
			std::unordered_map<std::string, std::string, string_hash, std::equal_to<>> escape_values;

			inline bool has_value(std::string_view key) const {
				return escape_values.find(key) != escape_values.end();
			}

			inline decltype(escape_values)::const_iterator find_value(std::string_view key) const {
				return escape_values.find(key);
			}

			inline decltype(escape_values)::const_iterator begin() const {
				return escape_values.begin();
			}

			inline decltype(escape_values)::const_iterator end() const {
				return escape_values.end();
			}
		};

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

		bool parse_csv(bool handle_strings = false);

		void add_escape_value(std::string_view key, std::string_view value);
		void remove_escape_value(std::string_view key, std::string_view value);
		void clear_escape_values();

		const std::vector<csv::LineObject>& get_lines() const;

		Parser(Parser&&);
		Parser& operator=(Parser&&);

		~Parser();

	private:
		class BufferHandler;
		std::unique_ptr<BufferHandler> _buffer_handler;
		std::vector<csv::LineObject> _lines;
		State _parser_state;

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<BufferHandler, Args...> auto func, Args... args);
	};

	using Windows1252Parser = Parser<EncodingType::Windows1252>;
	using Utf8Parser = Parser<EncodingType::Utf8>;
}