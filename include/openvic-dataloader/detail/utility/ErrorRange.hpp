#pragma once

#include <utility>

#include <openvic-dataloader/Error.hpp>

#include <dryad/node.hpp>

namespace ovdl::detail {
	using error_range = decltype(std::declval<const error::Root*>()->errors());
}