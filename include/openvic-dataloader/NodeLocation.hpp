#pragma once

#include <cstdint>

namespace ovdl {
	struct NodeLocation {
		const char* _begin = nullptr;
		const char* _end = nullptr;

		NodeLocation();
		NodeLocation(const char* pos);
		NodeLocation(const char* begin, const char* end);

		NodeLocation(const NodeLocation&) noexcept;
		NodeLocation& operator=(const NodeLocation&);

		NodeLocation(NodeLocation&&);
		NodeLocation& operator=(NodeLocation&&);

		const char* begin() const;
		const char* end() const;

		bool is_synthesized() const;

		static NodeLocation make_from(const char* begin, const char* end);
	};

	struct FilePosition {
		std::uint32_t start_line = std::uint32_t(-1), end_line = std::uint32_t(-1), start_column = std::uint32_t(-1), end_column = std::uint32_t(-1);

		inline constexpr bool is_empty() { return start_line == std::uint32_t(-1) && end_line == std::uint32_t(-1) && start_column == std::uint32_t(-1) && end_column == std::uint32_t(-1); }
	};
}