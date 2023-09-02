#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include <openvic-dataloader/detail/VectorConstexpr.hpp>

namespace ovdl::csv {
	/// LineObject should be able to recognize the differences between:
	///	Input -> Indexes == ""
	/// ;;a;b;c;; -> 0,1,5,6+ == ""
	/// a;b;c -> 3+ == ""
	/// a;;b;c;; -> 1,4,5+ == ""
	/// a;b;;c; -> 2,4+ == ""
	/// a;b;c;; -> 3,4+ == ""
	/// a;b;c; -> 3+ == ""
	/// ;a;b;c -> 0,4+ == ""
	///
	/// If this is incorrect, please report an issue.
	class LineObject final : public std::vector<std::tuple<std::uint32_t, std::string>> {
	public:
		// Stored position of value
		using position_type = std::uint32_t;
		// Value
		using inner_value_type = std::string;
		using container_type = std::vector<std::tuple<position_type, inner_value_type>>;

		OVDL_VECTOR_CONSTEXPR LineObject() = default;
		OVDL_VECTOR_CONSTEXPR LineObject(LineObject&) = default;
		OVDL_VECTOR_CONSTEXPR LineObject(LineObject&&) = default;
		OVDL_VECTOR_CONSTEXPR LineObject(const LineObject&) = default;

		OVDL_VECTOR_CONSTEXPR LineObject& operator=(const LineObject& other) = default;
		OVDL_VECTOR_CONSTEXPR LineObject& operator=(LineObject&& other) = default;

		OVDL_VECTOR_CONSTEXPR ~LineObject() = default;

		OVDL_VECTOR_CONSTEXPR LineObject(std::initializer_list<value_type> pos_and_val) : container_type(pos_and_val) {
		}

		OVDL_VECTOR_CONSTEXPR LineObject(position_type prefix_end, std::initializer_list<value_type> pos_and_val, position_type suffix_end = 0)
			: container_type(pos_and_val),
			  _prefix_end(prefix_end),
			  _suffix_end(suffix_end) {
		}

		/// Special Functionality
		/// Retrieves value, produces "" for empty values
		constexpr std::string_view get_value_for(std::size_t position) const {
			if (position <= _prefix_end || position >= _suffix_end) return "";
			for (const auto& [pos, val] : *this) {
				if (pos == position) return val;
			}
			return "";
		}
		/// Tries to retrieve reference, produces nullopt for empty values
		constexpr std::optional<const std::reference_wrapper<const std::string>> try_get_string_at(std::size_t position) const {
			if (position <= _prefix_end || position > _suffix_end) return std::nullopt;
			for (const auto& [pos, val] : *this) {
				if (pos == position) return std::cref(val);
			}
			return std::nullopt;
		}

		constexpr position_type prefix_end() const { return _prefix_end; }
		constexpr void set_prefix_end(position_type value) { _prefix_end = value; }

		constexpr position_type suffix_end() const { return _suffix_end; }
		constexpr void set_suffix_end(position_type value) { _suffix_end = value; }

		constexpr std::size_t value_count() const { return _suffix_end; }

	private:
		// Should be position of first valid value on line
		position_type _prefix_end = 0;
		// Should be position after last value or position after last seperator
		position_type _suffix_end = 0;
	};
}