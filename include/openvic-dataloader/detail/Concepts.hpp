#pragma once

#include <concepts>
#include <cstdint>
#include <functional>
#include <utility>

namespace ovdl {
	struct File;
	namespace detail {
		enum class buffer_error : std::uint8_t;
	}
}

namespace ovdl::detail {
	template<typename T, typename... Ts>
	concept any_of = std::disjunction_v<std::is_same<T, Ts>...>;

	template<typename T>
	concept HasCstr =
		requires(T t) {
			{ t.c_str() } -> std::same_as<const char*>;
		};

	template<typename T>
	concept HasPath = requires(T& t) {
		{ t.path() } -> std::same_as<const char*>;
	};

	template<typename T, typename Self, typename... Args>
	concept LoadCallback =
		requires(T&& t, Self&& self, Args&&... args) {
			{ std::invoke(std::forward<T>(t), std::forward<Self>(self), std::forward<Args>(args)...) } -> std::same_as<ovdl::detail::buffer_error>;
		};

	template<typename T>
	concept IsEncoding = requires(T t) {
		typename T::char_type;
		typename T::int_type;
		{ T::template is_secondary_char_type<typename T::char_type>() } -> std::same_as<bool>;
		{ T::eof() } -> std::same_as<typename T::int_type>;
		{ T::to_int_type(typename T::char_type {}) } -> std::same_as<typename T::int_type>;
	};

	template<typename T, typename R, typename... Args>
	concept Invocable_R = std::invocable<T, Args...> && requires(Args&&... args) {
		{ invoke(forward<Args>(args)...) } -> std::convertible_to<R>;
	};
}