#pragma once

#include <cstdint>

namespace ovdl {
	template<typename CharT>
	struct BasicNodeLocation {
		using char_type = CharT;

		const char_type* _begin = nullptr;
		const char_type* _end = nullptr;

		BasicNodeLocation() = default;
		BasicNodeLocation(const char_type* pos)
			: _begin(pos),
			  _end(pos) {}
		BasicNodeLocation(const char_type* begin, const char_type* end)
			: _begin(begin),
			  _end(end) {}

		BasicNodeLocation(const BasicNodeLocation&) noexcept = default;
		BasicNodeLocation& operator=(const BasicNodeLocation&) = default;

		BasicNodeLocation(BasicNodeLocation&&) = default;
		BasicNodeLocation& operator=(BasicNodeLocation&&) = default;

		template<typename OtherCharT>
		void set_from(const BasicNodeLocation<OtherCharT>& other) {
			if constexpr (sizeof(CharT) <= sizeof(OtherCharT)) {
				_begin = reinterpret_cast<const CharT*>(other.begin());
				if (other.begin() == other.end()) {
					_end = _begin;
				} else {
					_end = reinterpret_cast<const CharT*>(other.end()) + (sizeof(OtherCharT) - sizeof(CharT));
				}
			} else {
				_begin = reinterpret_cast<const CharT*>(other.begin());
				if (other.end() - other.begin() <= 0) {
					_end = reinterpret_cast<const CharT*>(other.begin());
				} else {
					_end = reinterpret_cast<const CharT*>(other.end() - (sizeof(CharT) - sizeof(OtherCharT)));
				}
			}
		}

		template<typename OtherCharT>
		BasicNodeLocation(const BasicNodeLocation<OtherCharT>& other) {
			set_from(other);
		}

		template<typename OtherCharT>
		BasicNodeLocation& operator=(const BasicNodeLocation<OtherCharT>& other) {
			set_from(other);
			return *this;
		}

		const char_type* begin() const { return _begin; }
		const char_type* end() const { return _end; }

		bool is_synthesized() const { return _begin == nullptr && _end == nullptr; }

		static BasicNodeLocation make_from(const char_type* begin, const char_type* end) {
			end++;
			if (begin >= end) {
				return BasicNodeLocation(begin);
			}
			return BasicNodeLocation(begin, end);
		}
	};

	using NodeLocation = BasicNodeLocation<char>;

	struct FilePosition {
		std::uint32_t start_line = std::uint32_t(-1), end_line = std::uint32_t(-1), start_column = std::uint32_t(-1), end_column = std::uint32_t(-1);

		inline constexpr bool is_empty() { return start_line == std::uint32_t(-1) && end_line == std::uint32_t(-1) && start_column == std::uint32_t(-1) && end_column == std::uint32_t(-1); }
	};
}