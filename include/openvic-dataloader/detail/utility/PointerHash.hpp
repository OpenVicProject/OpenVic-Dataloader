#pragma once

#include <cstdint>

namespace ovdl::detail {
	/* hash any pointer */
	template<typename T>
	struct PointerHash {
		using type = T;
		using ptr_type = T*;
		using const_type = const T;
		using const_ptr_type = const T*;
		using const_ptr_const_type = const const_ptr_type;
		constexpr std::size_t operator()(const_ptr_const_type pointer) const {
			auto addr = reinterpret_cast<uintptr_t>(pointer);
#if SIZE_MAX < UINTPTR_MAX
			/* size_t is not large enough to hold the pointer’s memory address */
			addr %= SIZE_MAX; /* truncate the address so it is small enough to fit in a size_t */
#endif
			return addr;
		}
	};
}