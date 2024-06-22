#pragma once

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <optional>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <openvic-dataloader/detail/Constexpr.hpp>

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
	class LineObject final : public std::vector<std::pair<std::uint32_t, std::string>> {
	public:
		// Stored position of value
		using position_type = std::uint32_t;
		// Value
		using inner_value_type = std::string;
		using container_type = std::vector<std::pair<position_type, inner_value_type>>;

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
			if (position < _prefix_end || position >= _suffix_end) return "";
			for (const auto& [pos, val] : *this) {
				if (pos == position) return val;
			}
			return "";
		}
		/// Tries to retrieve reference, produces nullopt for empty values
		constexpr std::optional<const std::reference_wrapper<const std::string>> try_get_string_at(std::size_t position) const {
			if (position < _prefix_end || position >= _suffix_end) return std::nullopt;
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

		struct SepTransformer {
			const LineObject& line_object;
			std::string_view separator;
		};

		constexpr SepTransformer use_sep(std::string_view seperator) const {
			return { *this, seperator };
		}

	private:
		// Should be position of first valid value on line
		position_type _prefix_end = 0;
		// Should be position after last value or position after last seperator
		position_type _suffix_end = 0;
	};

	struct VectorSepTransformer {
		const std::vector<LineObject>& vector;
		std::string_view separator;
	};

	constexpr VectorSepTransformer use_sep(const std::vector<LineObject>& vector, std::string_view separator) {
		return { vector, separator };
	}

	inline std::ostream& operator<<(std::ostream& stream, const LineObject& line) {
		static constexpr char SEP = ';';
		LineObject::position_type sep_index = 0;
		for (const auto& [pos, val] : line) {
			while (sep_index < pos) {
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

	inline std::ostream& operator<<(std::ostream& stream, const LineObject::SepTransformer& transformer) {
		auto quote_check = [&transformer, is_one = transformer.separator.size() == 1](const std::string_view str) {
			if (is_one) {
				char SEP = transformer.separator[0];
				return std::any_of(str.begin(), str.end(), [SEP](char c) { return c == SEP || std::isspace(c); });
			}
			return std::any_of(str.begin(), str.end(), [](char c) { return std::isspace(c); }) ||
				   str.find(transformer.separator) != std::string::npos;
		};

		LineObject::position_type sep_index = 0;
		for (const auto& [pos, val] : transformer.line_object) {
			while (sep_index < pos) {
				stream << transformer.separator;
				sep_index++;
			}
			if (quote_check(val)) {
				stream << '"' << val << '"';
			} else {
				stream << val;
			}
		}
		return stream;
	}

	inline std::ostream& operator<<(std::ostream& stream, const VectorSepTransformer& transformer) {
		for (const LineObject& line : transformer.vector) {
			stream << line.use_sep(transformer.separator) << '\n';
		}
		return stream;
	}
}