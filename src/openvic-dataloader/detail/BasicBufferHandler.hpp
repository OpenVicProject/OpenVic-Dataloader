#pragma once

#include <optional>

#include <openvic-dataloader/ParseError.hpp>
#include <openvic-dataloader/detail/OptionalConstexpr.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input/file.hpp>

#include "detail/Errors.hpp"

namespace ovdl::detail {
	template<typename Encoding = lexy::default_encoding, typename MemoryResource = void>
	class BasicBufferHandler {
	public:
		using encoding_type = Encoding;

		OVDL_OPTIONAL_CONSTEXPR bool is_valid() const {
			return _buffer.size() != 0;
		}

		OVDL_OPTIONAL_CONSTEXPR std::optional<ovdl::ParseError> load_buffer_size(const char* data, std::size_t size) {
			_buffer = lexy::buffer<Encoding, MemoryResource>(data, size);
			return std::nullopt;
		}

		OVDL_OPTIONAL_CONSTEXPR std::optional<ovdl::ParseError> load_buffer(const char* start, const char* end) {
			_buffer = lexy::buffer<Encoding, MemoryResource>(start, end);
			return std::nullopt;
		}

		std::optional<ovdl::ParseError> load_file(const char* path) {
			auto file = lexy::read_file<Encoding, lexy::encoding_endianness::bom, MemoryResource>(path);
			if (!file) {
				return ovdl::errors::make_no_file_error(path);
			}

			_buffer = file.buffer();
			return std::nullopt;
		}

		const auto& get_buffer() const {
			return _buffer;
		}

	protected:
		lexy::buffer<Encoding, MemoryResource> _buffer;
	};
}