#pragma once

#include <concepts>
#include <optional>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/ParseError.hpp>

namespace ovdl::detail {
	template<typename T, typename Self, typename... Args>
	concept LoadCallback =
		requires(T t, Self* self, Args... args) {
			{ t(self, std::forward<Args>(args)...) } -> std::same_as<std::optional<ParseError>>;
		};

	template<typename T>
	concept Has_c_str =
		requires(T t) {
			{ t.c_str() } -> std::same_as<const char*>;
		};
}