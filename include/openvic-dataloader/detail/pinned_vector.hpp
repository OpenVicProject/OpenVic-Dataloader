#pragma once

#include <vmcontainer/detail.hpp>
#include <vmcontainer/pinned_vector.hpp>

namespace ovdl::detail {
	static constexpr auto max_elements = mknejp::vmcontainer::max_elements;
	static constexpr auto max_bytes = mknejp::vmcontainer::max_bytes;
	static constexpr auto max_pages = mknejp::vmcontainer::max_pages;

	using pinned_vector_traits = mknejp::vmcontainer::pinned_vector_traits;

	template<typename T, typename Traits = pinned_vector_traits>
	using pinned_vector = mknejp::vmcontainer::pinned_vector<T, Traits>;
}