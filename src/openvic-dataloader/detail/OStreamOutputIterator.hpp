#pragma once

#include <ostream>

namespace ovdl::detail {
	struct OStreamOutputItertaor {
		std::reference_wrapper<std::ostream> _stream;

		auto operator*() const noexcept {
			return *this;
		}
		auto operator++(int) const noexcept {
			return *this;
		}

		OStreamOutputItertaor& operator=(char c) {
			_stream.get().put(c);
			return *this;
		}
	};
}