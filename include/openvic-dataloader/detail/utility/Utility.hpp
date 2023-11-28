#pragma once

#include <string_view>
#include <type_traits>

#include "openvic-dataloader/detail/utility/TypeName.hpp"

namespace ovdl::detail {
	[[noreturn]] inline void unreachable() {
		// Uses compiler specific extensions if possible.
		// Even if no extension is used, undefined behavior is still raised by
		// an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
		__builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
		__assume(false);
#endif
	}

	template<typename Kind>
	constexpr std::string_view get_kind_name() {
		constexpr auto name = type_name<Kind>();

		return name;
	}

	template<typename EnumT>
		requires std::is_enum_v<EnumT>
	constexpr std::underlying_type_t<EnumT> to_underlying(EnumT e) {
		return static_cast<std::underlying_type_t<EnumT>>(e);
	}

	template<typename EnumT>
		requires std::is_enum_v<EnumT>
	constexpr EnumT from_underlying(std::underlying_type_t<EnumT> ut) {
		return static_cast<EnumT>(ut);
	}
}