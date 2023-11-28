#pragma once

#include <utility>

#include <openvic-dataloader/ParseState.hpp>
#include <openvic-dataloader/detail/utility/Concepts.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "detail/BufferError.hpp"

namespace ovdl::detail {
	template<typename Derived>
	struct ParseHandler {
		std::string make_error_from(buffer_error error) {
			switch (error) {
				using enum ovdl::detail::buffer_error;
				case buffer_is_null:
					return "Buffer could not be loaded.";
				case os_error:
					return "OS file error.";
				case file_not_found:
					return "File not found.";
				case permission_denied:
					return "Read Permission denied.";
				default:
					return "";
			}
		}

		template<typename... Args>
		constexpr void _run_load_func(detail::LoadCallback<Derived, Args...> auto func, Args... args);
	};

	template<IsFileParseState ParseState, typename MemoryResource = void>
	struct BasicFileParseHandler : ParseHandler<BasicFileParseHandler<ParseState, MemoryResource>> {
		using parse_state_type = ParseState;
		using encoding_type = typename parse_state_type::file_type::encoding_type;

		constexpr bool is_valid() const {
			if (!_parse_state) return false;
			return buffer().data() != nullptr;
		}

		constexpr buffer_error load_buffer_size(const char* data, std::size_t size) {
			lexy::buffer<encoding_type, MemoryResource> buffer(data, size);
			if (buffer.data() == nullptr) return buffer_error::buffer_is_null;
			_parse_state.reset(new parse_state_type { std::move(buffer) });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		constexpr buffer_error load_buffer(const char* start, const char* end) {
			lexy::buffer<encoding_type, MemoryResource> buffer(start, end);
			if (buffer.data() == nullptr) return buffer_error::buffer_is_null;
			_parse_state.reset(new parse_state_type { std::move(buffer) });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		buffer_error load_file(const char* path) {
			lexy::read_file_result file = lexy::read_file<encoding_type, lexy::encoding_endianness::bom, MemoryResource>(path);
			if (!file) {
				_parse_state.reset(new parse_state_type { path, lexy::buffer<typename parse_state_type::file_type::encoding_type>() });
				return ovdl::detail::from_underlying<buffer_error>(ovdl::detail::to_underlying(file.error()));
			}
			_parse_state.reset(new parse_state_type { path, std::move(file).buffer() });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		const char* path() const {
			if (!_parse_state) return "";
			return _parse_state->file().path();
		}

		parse_state_type& parse_state() {
			return *_parse_state;
		}

		const parse_state_type& parse_state() const {
			return *_parse_state;
		}

		constexpr const auto& buffer() const {
			return _parse_state->file().buffer();
		}

	protected:
		std::unique_ptr<parse_state_type> _parse_state;
	};

	template<IsParseState ParseState, typename MemoryResource = void>
	struct BasicStateParseHandler : ParseHandler<BasicStateParseHandler<ParseState, MemoryResource>> {
		using parse_state_type = ParseState;
		using encoding_type = typename parse_state_type::ast_type::file_type::encoding_type;

		constexpr bool is_valid() const {
			if (!_parse_state) return false;
			return buffer().data() != nullptr;
		}

		constexpr buffer_error load_buffer_size(const char* data, std::size_t size) {
			lexy::buffer<encoding_type, MemoryResource> buffer(data, size);
			_parse_state.reset(new parse_state_type { std::move(buffer) });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		constexpr buffer_error load_buffer(const char* start, const char* end) {
			lexy::buffer<encoding_type, MemoryResource> buffer(start, end);
			_parse_state.reset(new parse_state_type { std::move(buffer) });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		buffer_error load_file(const char* path) {
			lexy::read_file_result file = lexy::read_file<encoding_type, lexy::encoding_endianness::bom, MemoryResource>(path);
			if (!file) {
				_parse_state.reset(new parse_state_type { path, lexy::buffer<typename parse_state_type::ast_type::file_type::encoding_type>() });
				return ovdl::detail::from_underlying<buffer_error>(ovdl::detail::to_underlying(file.error()));
			}

			_parse_state.reset(new parse_state_type { path, std::move(file).buffer() });
			return is_valid() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		const char* path() const {
			if (!_parse_state) return "";
			return _parse_state->ast().file().path();
		}

		parse_state_type& parse_state() {
			return *_parse_state;
		}

		const parse_state_type& parse_state() const {
			return *_parse_state;
		}

		constexpr const auto& buffer() const {
			return _parse_state->ast().file().buffer();
		}

	protected:
		std::unique_ptr<parse_state_type> _parse_state;
	};
}