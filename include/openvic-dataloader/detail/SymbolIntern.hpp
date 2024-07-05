#pragma once

#include <cstdint>
#include <iterator>
#include <string_view>

#include <openvic-dataloader/detail/pinned_vector.hpp>

#include <dryad/symbol.hpp>

namespace ovdl {
	// Contains all unique symbols, null-terminated, in memory one after the other.
	template<typename CharT>
	struct symbol_buffer {
		static constexpr auto min_buffer_size = 16 * 1024;

		constexpr symbol_buffer() : _data_buffer(ovdl::detail::max_elements(min_buffer_size + 1)) {}
		explicit symbol_buffer(std::size_t max_elements)
			: _data_buffer(ovdl::detail::max_elements(std::max<std::size_t>(max_elements, min_buffer_size + 1))) {
			_data_buffer.reserve(min_buffer_size);
		}

		void free() {
		}

		bool reserve(std::size_t new_capacity) {
			if (new_capacity <= _data_buffer.capacity())
				return true;

			if (new_capacity >= _data_buffer.max_size()) {
				_data_buffer.reserve(_data_buffer.max_size());
				return false;
			}

			_data_buffer.reserve(new_capacity * sizeof(CharT));

			return true;
		}

		bool reserve_new_string(std::size_t new_string_length) {
			// +1 for null-terminator.
			auto new_size = _data_buffer.size() + new_string_length + 1;
			if (new_size <= _data_buffer.capacity())
				return true;

			auto new_capacity = new_size * 2;
			if (new_capacity < min_buffer_size)
				new_capacity = min_buffer_size;

			if (!reserve(new_capacity)) {
				return _data_buffer.capacity() >= new_size;
			}

			return true;
		}

		const CharT* insert(const CharT* str, std::size_t length) {
			DRYAD_PRECONDITION(_data_buffer.capacity() - _data_buffer.size() >= length + 1);

			auto index = _data_buffer.cend();

			_data_buffer.insert(_data_buffer.cend(), str, str + (length * sizeof(CharT)));
			_data_buffer.push_back(CharT(0));

			return index;
		}

		const CharT* c_str(std::size_t index) const {
			DRYAD_PRECONDITION(index < _data_buffer.size());
			return _data_buffer.data() + index;
		}

		std::size_t size() const {
			return _data_buffer.size();
		}

		std::size_t capacity() const {
			return _data_buffer.capacity();
		}

		std::size_t max_size() const {
			return _data_buffer.max_size();
		}

	private:
		detail::pinned_vector<CharT> _data_buffer;
	};

	template<typename IndexType, typename CharT>
	struct symbol_index_hash_traits {
		const symbol_buffer<CharT>* buffer;

		using value_type = IndexType;

		struct string_view {
			const CharT* ptr;
			std::size_t length;
		};

		static constexpr bool is_unoccupied(IndexType index) {
			return index == IndexType(-1);
		}
		static void fill_unoccupied(IndexType* data, std::size_t size) {
			// It has all bits set to 1, so we can do it per-byte.
			std::memset(data, static_cast<unsigned char>(-1), size * sizeof(IndexType));
		}

		static constexpr bool is_equal(IndexType entry, IndexType value) {
			return entry == value;
		}
		bool is_equal(IndexType entry, string_view str) const {
			auto existing_str = buffer->c_str(entry);
			return std::strncmp(existing_str, str.ptr, str.length) == 0 && *(existing_str + str.length) == CharT(0);
		}

		std::size_t hash(IndexType entry) const {
			auto str = buffer->c_str(entry);
			return dryad::default_hash_algorithm().hash_c_str(str).finish();
		}
		static constexpr std::size_t hash(string_view str) {
			return dryad::default_hash_algorithm()
				.hash_bytes(reinterpret_cast<const unsigned char*>(str.ptr), str.length * sizeof(CharT))
				.finish();
		}
	};

	template<typename CharT = char>
	class symbol;

	template<typename Id, typename CharT = char, typename IndexType = std::size_t,
		typename MemoryResource = void>
	class symbol_interner {
		static_assert(std::is_trivial_v<CharT>);
		static_assert(std::is_unsigned_v<IndexType>);

		using resource_ptr = dryad::_detail::memory_resource_ptr<MemoryResource>;
		using traits = symbol_index_hash_traits<IndexType, CharT>;

	public:
		using symbol = ovdl::symbol<CharT>;

		//=== construction ===//
		constexpr symbol_interner() : _resource(dryad::_detail::get_memory_resource<MemoryResource>()) {}
		constexpr explicit symbol_interner(std::size_t max_elements)
			: _buffer(max_elements),
			  _resource(dryad::_detail::get_memory_resource<MemoryResource>()) {}
		constexpr explicit symbol_interner(std::size_t max_elements, MemoryResource* resource)
			: _buffer(max_elements),
			  _resource(resource) {}

		~symbol_interner() noexcept {
			_buffer.free();
			_map.free(_resource);
		}

		symbol_interner(symbol_interner&& other) noexcept
			: _buffer(other._buffer), _map(other._map), _resource(other._resource) {
			other._buffer = {};
			other._map = {};
		}

		symbol_interner& operator=(symbol_interner&& other) noexcept {
			dryad::_detail::swap(_buffer, other._buffer);
			dryad::_detail::swap(_map, other._map);
			dryad::_detail::swap(_resource, other._resource);
			return *this;
		}

		//=== interning ===//
		bool reserve(std::size_t number_of_symbols, std::size_t average_symbol_length) {
			auto success = _buffer.reserve(number_of_symbols * average_symbol_length);
			_map.rehash(_resource, _map.to_table_capacity(number_of_symbols), traits { &_buffer });
			return success;
		}

		symbol intern(const CharT* str, std::size_t length) {
			if (_map.should_rehash())
				_map.rehash(_resource, traits { &_buffer });

			auto entry = _map.lookup_entry(typename traits::string_view { str, length }, traits { &_buffer });
			if (entry)
				// Already interned, return index.
				return symbol(_buffer.c_str(entry.get()));

			// Copy string data to buffer, as we don't have it yet.
			if (!_buffer.reserve_new_string(length)) // Ran out of virtual memory space
				return symbol();

			auto begin = _buffer.insert(str, length);
			auto idx = std::distance(_buffer.c_str(0), begin);
			DRYAD_PRECONDITION(idx == IndexType(idx)); // Overflow of index type.

			// Store index in map.
			entry.create(IndexType(idx));

			// Return new symbol.
			return symbol(begin);
		}
		template<std::size_t N>
		symbol intern(const CharT (&literal)[N]) {
			DRYAD_PRECONDITION(literal[N - 1] == CharT(0));
			return intern(literal, N - 1);
		}

	private:
		symbol_buffer<CharT> _buffer;
		dryad::_detail::hash_table<traits, 1024> _map;
		DRYAD_EMPTY_MEMBER resource_ptr _resource;

		friend symbol;
	};

	template<typename CharT>
	struct symbol {
		using char_type = CharT;

		constexpr symbol() = default;
		constexpr explicit symbol(const CharT* begin) : _begin(begin) {}

		constexpr explicit operator bool() const {
			return _begin != nullptr;
		}

		constexpr const CharT* c_str() const {
			return _begin;
		}

		constexpr const std::basic_string_view<CharT> view() const {
			return _begin;
		}

		//=== comparison ===//
		friend constexpr bool operator==(symbol lhs, symbol rhs) {
			return lhs._begin == rhs._begin;
		}
		friend constexpr bool operator!=(symbol lhs, symbol rhs) {
			return lhs._begin != rhs._begin;
		}

		friend constexpr bool operator<(symbol lhs, symbol rhs) {
			return lhs._begin < rhs._begin;
		}
		friend constexpr bool operator<=(symbol lhs, symbol rhs) {
			return lhs._begin <= rhs._begin;
		}
		friend constexpr bool operator>(symbol lhs, symbol rhs) {
			return lhs._begin > rhs._begin;
		}
		friend constexpr bool operator>=(symbol lhs, symbol rhs) {
			return lhs._begin >= rhs._begin;
		}

	private:
		const CharT* _begin = nullptr;

		template<typename, typename, typename, typename>
		friend class symbol_interner;
	};

	struct SymbolIntern {
		struct SymbolId;
		using index_type = std::uint32_t;
		using symbol_type = symbol<char>;
		using symbol_interner_type = symbol_interner<SymbolId, symbol_type::char_type, index_type>;
	};
}