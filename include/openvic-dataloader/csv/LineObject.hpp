#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <set>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <openvic-dataloader/csv/ValueNode.hpp>
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
	class LineObject final : public std::vector<ValueNode> {
	public:
		// Stored position of value
		using position_type = ValueNode::position_type;
		// Value
		using inner_value_type = ValueNode;
		using container_type = std::vector<ValueNode>;

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
		constexpr std::string get_value_for(std::size_t position) const {
			if (position < _prefix_end || position >= _suffix_end) return "";
			for (const auto& object : *this) {
				if (object.get_position() == position) return object.make();
			}
			return "";
		}
		/// Tries to retrieve string, produces nullopt for empty values
		constexpr std::optional<const std::string> try_get_string_at(std::size_t position) const {
			if (position < _prefix_end || position >= _suffix_end) return std::nullopt;
			for (const auto& object : *this) {
				if (object.get_position() == position) return object.make();
			}
			return std::nullopt;
		}
		constexpr std::optional<const std::reference_wrapper<const ValueNode>> try_get_object_at(std::size_t position) const {
			if (position < _prefix_end || position >= _suffix_end) return std::nullopt;
			for (const auto& object : *this) {
				if (object.get_position() == position) return object;
			}
			return std::nullopt;
		}

		/// Retrieves value, produces "" for empty values
		constexpr std::string_view get_value_for(std::size_t position, const IsMap<std::string> auto& map) const {
			if (position < _prefix_end || position >= _suffix_end) return "";
			for (const auto& object : *this) {
				if (object.get_position() == position) return object.make_from_map(map);
			}
			return "";
		}
		/// Tries to retrieve string, produces nullopt for empty values
		constexpr std::optional<const std::string> try_get_string_at(std::size_t position, const IsMap<std::string> auto& map) const {
			if (position < _prefix_end || position >= _suffix_end) return std::nullopt;
			for (const auto& object : *this) {
				if (object.get_position() == position) return object.make_from_map(map);
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

	inline std::ostream& operator<<(std::ostream& stream, const LineObject& line) {
		static const char SEP = ';';
		LineObject::position_type sep_index = 0;
		for (const auto& object : line) {
			const std::string& val = object.make();
			while (sep_index < object.get_position()) {
				stream << SEP;
				sep_index++;
			}
			if (std::any_of(val.begin(), val.end(), [](char c) { return c == SEP || std::isspace(c); })) {
				stream << '"' << val << '"';
			} else {
				stream << val;
			}
		}
		return stream;
	}

	inline std::ostream& operator<<(std::ostream& stream, const std::vector<LineObject>& lines) {
		for (const LineObject& line : lines) {
			stream << line << '\n';
		}
		return stream;
	}
}