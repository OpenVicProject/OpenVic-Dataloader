#pragma once

#include <utility>

#include <dryad/node.hpp>

namespace ovdl::detail {
	template<typename ErrorRoot>
	using error_range = decltype(std::declval<const ErrorRoot*>()->errors());
}