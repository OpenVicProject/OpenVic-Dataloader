#pragma once

#include <type_traits>

namespace ovdl::detail {
#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma clang diagnostic ignored "-Wunknown-warning-option"
#pragma GCC diagnostic ignored "-Wnon-template-friend"
#endif
	template<typename T>
	struct Reader {
		friend auto adl_GetSelfType(Reader<T>);
	};

	template<typename T, typename U>
	struct Writer {
		friend auto adl_GetSelfType(Reader<T>) { return U {}; }
	};
#if !defined(_MSC_VER)
#pragma GCC diagnostic pop
#endif

	inline void adl_GetSelfType() {}

	template<typename T>
	using Read = std::remove_pointer_t<decltype(adl_GetSelfType(Reader<T> {}))>;
}
