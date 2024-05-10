#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

#include <lexy/_detail/config.hpp>
#include <lexy/_detail/integer_sequence.hpp>
#include <lexy/dsl/identifier.hpp>

namespace ovdl::detail {

	template<std::size_t N, typename CharT>
	struct string_literal;

	struct _string_literal {
	protected:
		static constexpr auto _to_string(const auto& input) { return string_literal(input); }
		static constexpr auto _concat(const auto&... input) { return string_literal(_to_string(input)...); }
	};

	template<std::size_t N, typename CharT = char>
	struct string_literal : _string_literal {
		using value_type = CharT;
		using pointer = value_type*;
		using const_pointer = const value_type*;
		using reference = value_type&;
		using const_reference = const value_type&;
		using size_type = std::size_t;
		using difference_type = std::ptrdiff_t;
		using traits_type = std::char_traits<value_type>;

		static constexpr size_type npos = size_type(-1);

		static constexpr auto size = std::integral_constant<std::size_t, N - 1> {};
		static constexpr auto length = size;

		value_type _data[N];

		constexpr string_literal() noexcept = default;

		constexpr string_literal(const value_type (&literal)[N]) noexcept {
			for (auto i = 0u; i != N; i++)
				_data[i] = literal[i];
		}

		constexpr string_literal(value_type c) noexcept : _data {} {
			_data[0] = c;
		}

		constexpr auto begin() noexcept { return std::begin(_data); }
		constexpr auto end() noexcept { return std::end(_data); }
		constexpr auto cbegin() const noexcept { return std::cbegin(_data); }
		constexpr auto cend() const noexcept { return std::cend(_data); }
		constexpr auto rbegin() noexcept { return std::rbegin(_data); }
		constexpr auto rend() noexcept { return std::rend(_data); }
		constexpr auto crbegin() const noexcept { return std::crbegin(_data); }
		constexpr auto crend() const noexcept { return std::crend(_data); }

		constexpr auto front() const noexcept { return as_string_view().front(); }
		constexpr auto back() const noexcept { return as_string_view().back(); }
		constexpr auto at(size_type pos) const { return as_string_view().at(pos); }

		constexpr auto operator[](size_type pos) const noexcept { return as_string_view()[pos]; }

		constexpr bool empty() const { return as_string_view().empty(); }

		template<size_type N2>
		constexpr bool operator==(const string_literal<N2, value_type>& other) const {
			return as_string_view() == other.as_string_view();
		}

		constexpr bool starts_with(std::basic_string_view<value_type> sv) const noexcept { return as_string_view().starts_with(sv); }
		constexpr bool starts_with(value_type ch) const noexcept { return as_string_view().starts_with(ch); }
		constexpr bool starts_with(const value_type* s) const { return as_string_view().starts_with(s); }

		template<size_type N2>
		constexpr bool starts_with(const string_literal<N2, value_type>& other) const {
			return starts_with(other.as_string_view());
		}

		constexpr bool ends_with(std::basic_string_view<value_type> sv) const noexcept { return as_string_view().ends_with(sv); }
		constexpr bool ends_with(value_type ch) const noexcept { return as_string_view().ends_with(ch); }
		constexpr bool ends_with(const value_type* s) const { return as_string_view().ends_with(s); }

		template<size_type N2>
		constexpr bool ends_with(const string_literal<N2, value_type>& other) const {
			return ends_with(other.as_string_view());
		}

		constexpr auto clear() const noexcept {
			return string_literal<0, value_type> {};
		}

		constexpr auto push_back(value_type c) const noexcept {
			return *this + string_literal { c };
		}

		constexpr auto pop_back() const noexcept {
			string_literal<N - 1, value_type> result {};
			for (auto i = 0u; i != N - 2; i++)
				result._data[i] = _data[i];
			return result;
		}

		template<size_type count>
		constexpr auto append(value_type ch) const noexcept {
			string_literal<N + count, value_type> result {};
			for (auto i = 0u; i != N; i++)
				result._data[i] = _data[i];

			for (auto i = N; i != N + count; i++)
				result._data[i] = ch;

			return result;
		}

		template<size_type N2>
		constexpr auto append(string_literal<N2, value_type> str) const noexcept {
			return *this + str;
		}

		template<size_type N2>
		constexpr auto append(const value_type (&literal)[N2]) const noexcept {
			return *this + literal;
		}

		template<size_type pos = 0, size_type count = npos>
		constexpr auto substr() const noexcept {
			static_assert(pos <= N, "pos must be less than or equal to N");
			constexpr size_type result_size = std::min(count - pos, N - pos);

			string_literal<result_size, value_type> result {};
			for (size_type i = 0u, i2 = pos; i != result_size; i++, i2++)
				result._data[i] = _data[i2];
			return result;
		}

		constexpr auto substr() const noexcept {
			return substr<>();
		}

		constexpr std::string_view substr(size_type pos, size_type count = npos) const noexcept {
			return as_string_view().substr(pos, count);
		}

		constexpr size_type find(std::string_view str, size_type pos = 0) const noexcept {
			return as_string_view().find(str, pos);
		}

		template<size_type N2>
		constexpr size_type find(const value_type (&literal)[N2], size_type pos = 0) const noexcept {
			return as_string_view().find(literal, pos, N2 - 2);
		}

		constexpr size_type rfind(std::string_view str, size_type pos = 0) const noexcept {
			return as_string_view().rfind(str, pos);
		}

		template<size_type N2>
		constexpr size_type rfind(const value_type (&literal)[N2], size_type pos = 0) const noexcept {
			return as_string_view().find(literal, pos, N2 - 2);
		}

		constexpr int compare(std::string_view str) const noexcept {
			return as_string_view().compare(str);
		}

		template<size_type N2>
		constexpr int compare(const value_type (&literal)[N2]) const noexcept {
			return as_string_view().compare(0, N, literal, N2 - 2);
		}

		constexpr operator std::basic_string_view<value_type>() const& { return as_string_view(); }
		constexpr operator std::basic_string_view<value_type>() const&& = delete;

		constexpr std::basic_string_view<value_type> as_string_view() const& { return std::basic_string_view<value_type>(_data, size() - 1); }
		constexpr std::basic_string_view<value_type> as_string_view() const&& = delete;

		constexpr operator const value_type*() const& { return c_str(); }
		constexpr operator const value_type*() const&& = delete;

		constexpr const value_type* c_str() const& { return _data; }
		constexpr const value_type* c_str() const&& = delete;

		constexpr const value_type* data() const& { return _data; }
		constexpr const value_type* data() const&& = delete;

		template<size_type N2>
		constexpr auto operator+(const string_literal<N2, value_type>& other) const noexcept {
			string_literal<N + N2 - 1, value_type> result {};
			for (size_type i = 0u; i != N; i++)
				result._data[i] = _data[i];

			for (size_type i = N - 1, i2 = 0; i2 != N2; i++, i2++)
				result._data[i] = other._data[i2];
			return result;
		}

		template<size_type N2>
		constexpr auto operator+(const value_type (&rhs)[N2]) const noexcept {
			return *this + _to_string(rhs);
		}

		template<size_type N2>
		friend constexpr auto operator+(const value_type (&lhs)[N2], string_literal<N, value_type> rhs) noexcept {
			return _to_string(lhs) + rhs;
		}
	};
	template<std::size_t N, typename CharT = char>
	string_literal(const CharT (&)[N]) -> string_literal<N, CharT>;

	template<typename CharT = char>
	string_literal(CharT) -> string_literal<1, CharT>;

	template<template<typename C, C... Cs> typename T, string_literal Str, std::size_t... Idx>
	auto _to_type_string(lexy::_detail::index_sequence<Idx...>) {
		return T<typename decltype(Str)::value_type, Str._data[Idx]...> {};
	}
	template<template<typename C, C... Cs> typename T, string_literal Str>
	using to_type_string = decltype(ovdl::detail::_to_type_string<T, Str>(lexy::_detail::make_index_sequence<decltype(Str)::size()> {}));
}

namespace ovdl::dsl {
	template<ovdl::detail::string_literal Str, typename L, typename T, typename... R>
	constexpr auto keyword(lexyd::_id<L, T, R...>) {
		return ovdl::detail::to_type_string<lexyd::_keyword<lexyd::_id<L, T>>::template get, Str> {};
	}
}