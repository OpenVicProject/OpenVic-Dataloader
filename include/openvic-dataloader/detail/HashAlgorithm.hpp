#pragma once

#include <cstdint>
#include <type_traits>

namespace ovdl::detail {
	/// FNV-1a 64 bit hash.
	class DefaultHash {
		static constexpr std::uint64_t fnv_basis = 14695981039346656037ull;
		static constexpr std::uint64_t fnv_prime = 1099511628211ull;

	public:
		explicit DefaultHash() : _hash(fnv_basis) {}

		DefaultHash(DefaultHash&&) = default;
		DefaultHash& operator=(DefaultHash&&) = default;

		~DefaultHash() = default;

		DefaultHash&& hash_bytes(const unsigned char* ptr, std::size_t size) {
			for (auto i = 0u; i != size; ++i) {
				_hash ^= ptr[i];
				_hash *= fnv_prime;
			}
			return static_cast<std ::remove_reference_t<decltype(*this)>&&>(*this);
		}

		template<typename T>
			requires std::is_scalar_v<T>
		DefaultHash&& hash_scalar(T value) {
			static_assert(!std::is_floating_point_v<T>,
				"you shouldn't use floats as keys for a hash table");
			hash_bytes(reinterpret_cast<unsigned char*>(&value), sizeof(T));
			return static_cast<std ::remove_reference_t<decltype(*this)>&&>(*this);
		}

		template<typename CharT>
		DefaultHash&& hash_c_str(const CharT* str) {
			while (*str != '\0') {
				hash_scalar(*str);
				++str;
			}
			return static_cast<std ::remove_reference_t<decltype(*this)>&&>(*this);
		}

		std::uint64_t finish() && {
			return _hash;
		}

	private:
		std::uint64_t _hash;
	};
}