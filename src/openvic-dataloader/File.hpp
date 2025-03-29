#pragma once

#include <cassert>
#include <concepts> // IWYU pragma: keep
#include <type_traits>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/_detail/config.hpp>
#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>

#include <dryad/node_map.hpp>

namespace ovdl {
	struct File {
		using buffer_ids = detail::TypeRegister<
			lexy::buffer<lexy::default_encoding, void>,
			lexy::buffer<lexy::ascii_encoding, void>,
			lexy::buffer<lexy::utf8_char_encoding, void>,
			lexy::buffer<lexy::utf8_encoding, void>,
			lexy::buffer<lexy::utf16_encoding, void>,
			lexy::buffer<lexy::utf32_encoding, void>,
			lexy::buffer<lexy::byte_encoding, void>>;

		File() = default;
		explicit File(const char* path);

		const char* path() const noexcept;

		bool is_valid() const noexcept;

		std::size_t size() const noexcept;

		lexy::buffer<lexy::utf8_char_encoding, void>& buffer() {
			return _buffer;
		}

		lexy::buffer<lexy::utf8_char_encoding, void> const& buffer() const {
			return _buffer;
		}

	protected:
		const char* _path = "";
		lexy::buffer<lexy::utf8_char_encoding, void> _buffer;
	};

	template<typename NodeT>
	struct BasicFile : File {
		using node_type = NodeT;

		BasicFile() = default;

		explicit BasicFile(const char* path, lexy::buffer<lexy::utf8_char_encoding, void>&& buffer)
			: File(path) {
			_buffer = static_cast<std::remove_reference_t<decltype(buffer)>&&>(buffer);
		}

		explicit BasicFile(lexy::buffer<lexy::utf8_char_encoding, void>&& buffer)
			: File("") {
			_buffer = static_cast<std::remove_reference_t<decltype(buffer)>&&>(buffer);
		}

		void set_location(const node_type* n, NodeLocation loc) {
			_map.insert(n, loc);
		}

		NodeLocation location_of(const node_type* n) const {
			auto result = _map.lookup(n);
			DRYAD_ASSERT(result != nullptr, "every Node should have a NodeLocation");
			return *result;
		}

	protected:
		dryad::node_map<const node_type, NodeLocation> _map;
	};
}