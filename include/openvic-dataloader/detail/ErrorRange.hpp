#pragma once

#include <utility>

namespace ovdl::detail {
	template<typename ErrorRoot>
	using error_range = decltype(std::declval<const ErrorRoot*>()->errors());
}