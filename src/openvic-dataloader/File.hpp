#pragma once

#include <cassert>
#include <concepts> // IWYU pragma: keep
#include <type_traits>
#include <variant>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>

#include <dryad/node_map.hpp>

namespace ovdl {
	struct File {
		using buffer_ids = detail::TypeRegister<
			lexy::buffer<lexy::default_encoding, void>,
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

		template<typename Encoding, typename MemoryResource = void>
		constexpr bool is_buffer() const {
			return buffer_ids::type_id<lexy::buffer<Encoding, MemoryResource>>() + 1 == _buffer.index();
		}

		template<typename Encoding, typename MemoryResource = void>
		lexy::buffer<Encoding, MemoryResource>* try_get_buffer_as() {
			return std::get_if<lexy::buffer<Encoding, MemoryResource>>(&_buffer);
		}

		template<typename Encoding, typename MemoryResource = void>
		const lexy::buffer<Encoding, MemoryResource>* try_get_buffer_as() const {
			return std::get_if<lexy::buffer<Encoding, MemoryResource>>(&_buffer);
		}

		template<typename Encoding, typename MemoryResource = void>
		lexy::buffer<Encoding, MemoryResource>& get_buffer_as() {
			assert((is_buffer<Encoding, MemoryResource>()));
			return *std::get_if<lexy::buffer<Encoding, MemoryResource>>(&_buffer);
		}

		template<typename Encoding, typename MemoryResource = void>
		const lexy::buffer<Encoding, MemoryResource>& get_buffer_as() const {
			assert((is_buffer<Encoding, MemoryResource>()));
			return *std::get_if<lexy::buffer<Encoding, MemoryResource>>(&_buffer);
		}

#define SWITCH_LIST \
	X(1)            \
	X(2)            \
	X(3)            \
	X(4)            \
	X(5)            \
	X(6)

#define X(NUM) \
	case NUM:  \
		return visitor(std::get<NUM>(_buffer));

		template<typename Visitor>
		decltype(auto) visit_buffer(Visitor&& visitor) {
			switch (_buffer.index()) {
				SWITCH_LIST
				case 0: return visitor(lexy::buffer<> {});
				default: ovdl::detail::unreachable();
			}
		}

		template<typename Return, typename Visitor>
		Return visit_buffer(Visitor&& visitor) {
			switch (_buffer.index()) {
				SWITCH_LIST
				case 0: return visitor(lexy::buffer<> {});
				default: ovdl::detail::unreachable();
			}
		}

		template<typename Visitor>
		decltype(auto) visit_buffer(Visitor&& visitor) const {
			switch (_buffer.index()) {
				SWITCH_LIST
				case 0: return visitor(lexy::buffer<> {});
				default: ovdl::detail::unreachable();
			}
		}

		template<typename Return, typename Visitor>
		Return visit_buffer(Visitor&& visitor) const {
			switch (_buffer.index()) {
				SWITCH_LIST
				case 0: return visitor(lexy::buffer<> {});
				default: ovdl::detail::unreachable();
			}
		}
#undef X
#undef SWITCH_LIST

	protected:
		const char* _path = "";
		std::size_t _buffer_size = 0;
		detail::type_prepend_t<buffer_ids::variant_type, std::monostate> _buffer;
	};

	template<typename NodeT>
	struct BasicFile : File {
		using node_type = NodeT;

		BasicFile() = default;

		template<typename Encoding, typename MemoryResource = void>
		explicit BasicFile(const char* path, lexy::buffer<Encoding, MemoryResource>&& buffer)
			: File(path) {
			_buffer_size = buffer.size();
			_buffer = static_cast<std::remove_reference_t<decltype(buffer)>&&>(buffer);
		}

		template<typename Encoding, typename MemoryResource = void>
		explicit BasicFile(lexy::buffer<Encoding, MemoryResource>&& buffer)
			: File("") {
			_buffer_size = buffer.size();
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