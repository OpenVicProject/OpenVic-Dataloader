#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include <openvic-dataloader/detail/Concepts.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "openvic-dataloader/detail/Encoding.hpp"
#include "openvic-dataloader/detail/Utility.hpp"

#include "detail/BufferError.hpp"
#include "detail/Convert.hpp"
#include "detail/Detect.hpp"
#include "detail/InternalConcepts.hpp"

namespace ovdl::detail {
	struct ParseHandler {
		std::string make_error_from(buffer_error error) const {
			switch (error) {
				using enum ovdl::detail::buffer_error;
				case buffer_is_null:
					return "Buffer could not be loaded.";
				case os_error:
					return "OS file error for '{}'.";
				case file_not_found:
					return "File '{}' not found.";
				case permission_denied:
					return "Read Permission for '{}' denied.";
				default:
					return "";
			}
		}

		constexpr bool is_valid() const {
			return is_valid_impl();
		}

		buffer_error load_buffer_size(const char* data, std::size_t size, std::optional<Encoding> fallback) {
			lexy::buffer<lexy::default_encoding> buffer(data, size);
			if (buffer.data() == nullptr) {
				return buffer_error::buffer_is_null;
			}
			return load_buffer_impl(std::move(buffer), "", fallback);
		}

		buffer_error load_buffer(const char* start, const char* end, std::optional<Encoding> fallback) {
			lexy::buffer<lexy::default_encoding> buffer(start, end);
			if (buffer.data() == nullptr) {
				return buffer_error::buffer_is_null;
			}
			return load_buffer_impl(std::move(buffer), "", fallback);
		}

		buffer_error load_file(const char* path, std::optional<Encoding> fallback) {
			lexy::read_file_result file = lexy::read_file<lexy::default_encoding, lexy::encoding_endianness::bom>(path);

			if (!file) {
				return ovdl::detail::from_underlying<buffer_error>(ovdl::detail::to_underlying(file.error()));
			}

			return load_buffer_impl(std::move(file).buffer(), path, fallback);
		}

		const char* path() const {
			return path_impl();
		}

		static Encoding get_system_fallback() {
			return _system_fallback_encoding.value_or(Encoding::Unknown);
		}

		virtual ~ParseHandler() = default;

	protected:
		constexpr virtual bool is_valid_impl() const = 0;
		constexpr virtual buffer_error load_buffer_impl(lexy::buffer<lexy::default_encoding>&& buffer, const char* path = "", std::optional<Encoding> fallback = std::nullopt) = 0;
		virtual const char* path_impl() const = 0;

		template<detail::IsStateType State, detail::IsEncoding BufferEncoding>
		static constexpr auto generate_state = [](State* state, const char* path, auto&& buffer, Encoding encoding) {
			if (path[0] != '\0') {
				*state = {
					path,
					lexy::buffer<BufferEncoding>(std::move(buffer)),
					encoding
				};
				return;
			}
			*state = { lexy::buffer<BufferEncoding>(std::move(buffer)), encoding };
		};

		template<detail::IsStateType State>
		static constexpr auto generate_conversion_state(State* state, const char* path, auto&& buffer, Encoding encoding) {
			size_t size = buffer.size();
			if (path[0] != '\0') {
				*state = {
					path,
					convert::make_buffer_from_raw<lexy::utf8_char_encoding>(encoding, std::move(buffer).release(), size),
					encoding
				};
				return;
			}
			*state = { convert::make_buffer_from_raw<lexy::utf8_char_encoding>(encoding, std::move(buffer).release(), size), encoding };
		};

		template<detail::IsStateType State>
		static void create_state(State* state, const char* path, lexy::buffer<lexy::default_encoding>&& buffer, std::optional<Encoding> fallback) {
			if (!_system_fallback_encoding.has_value()) {
				_detect_system_fallback_encoding();
			}
			bool is_bad_fallback = false;
			if (fallback.has_value()) {
				is_bad_fallback = fallback.value() == Encoding::Ascii || fallback.value() == Encoding::Utf8;
				if (is_bad_fallback) {
					fallback = _system_fallback_encoding.value();
				}
			} else {
				fallback = _system_fallback_encoding.value();
			}
			auto [encoding, is_alone] = encoding_detect::Detector { .default_fallback = fallback.value() }.detect_assess(buffer);
			switch (encoding) {
				using enum Encoding;
				case Ascii: {
					generate_state<State, lexy::ascii_encoding>(state, path, std::move(buffer), encoding);
					break;
				}
				case Utf8: {
					generate_state<State, lexy::utf8_char_encoding>(state, path, std::move(buffer), encoding);
					break;
				}
				case Unknown: {
					break;
				}
				case Windows1251:
				case Windows1252: {
					generate_conversion_state(state, path, std::move(buffer), encoding);
					break;
				}
				OVDL_DEFAULT_CASE_UNREACHABLE();
			}

			if (!is_alone) {
				state->logger().info("encoding type could not be distinguished");
			}

			if (is_bad_fallback) {
				state->logger().warning("fallback encoding cannot be ascii or utf8");
			}

			if (encoding == ovdl::detail::Encoding::Unknown) {
				state->logger().error("could not detect encoding");
			}
		}

	private:
		inline static std::optional<Encoding> _system_fallback_encoding = std::nullopt;
		static void _detect_system_fallback_encoding();
	};

	template<detail::IsFileParseState ParseState>
	struct BasicFileParseHandler : ParseHandler {
		using parse_state_type = ParseState;

		virtual constexpr bool is_valid_impl() const {
			return _parse_state.file().is_valid();
		}

		constexpr virtual buffer_error load_buffer_impl(lexy::buffer<lexy::default_encoding>&& buffer, const char* path, std::optional<Encoding> fallback) {
			if (buffer.data() == nullptr) {
				_parse_state = {};
				return buffer_error::buffer_is_null;
			}
			create_state(&_parse_state, path, std::move(buffer), fallback);
			return is_valid_impl() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		virtual const char* path_impl() const {
			return _parse_state.file().path();
		}

		parse_state_type& parse_state() {
			return _parse_state;
		}

		const parse_state_type& parse_state() const {
			return _parse_state;
		}

		template<typename Encoding>
		constexpr const auto& buffer() const {
			return _parse_state.file().template get_buffer_as<Encoding>();
		}

	protected:
		parse_state_type _parse_state;
	};

	template<detail::IsParseState ParseState>
	struct BasicStateParseHandler : ParseHandler {
		using parse_state_type = ParseState;

		virtual constexpr bool is_valid_impl() const {
			return _parse_state.ast().file().is_valid();
		}

		constexpr virtual buffer_error load_buffer_impl(lexy::buffer<lexy::default_encoding>&& buffer, const char* path, std::optional<Encoding> fallback) {
			if (buffer.data() == nullptr) {
				return buffer_error::buffer_is_null;
			}
			create_state(&_parse_state, path, std::move(buffer), fallback);
			return is_valid_impl() ? buffer_error::success : buffer_error::buffer_is_null;
		}

		virtual const char* path_impl() const {
			return _parse_state.ast().file().path();
		}

		parse_state_type& parse_state() {
			return _parse_state;
		}

		const parse_state_type& parse_state() const {
			return _parse_state;
		}

		template<typename Encoding>
		constexpr const auto& buffer() const {
			return _parse_state.ast().file().template get_buffer_as<Encoding>();
		}

	protected:
		parse_state_type _parse_state;
	};
}