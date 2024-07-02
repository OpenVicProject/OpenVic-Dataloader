#include <iterator>
#include <string_view>

#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/pinned_vector.hpp>

#include "Helper.hpp"
#include <snitch/snitch.hpp>

using namespace std::string_view_literals;

using symbol_buffer = ovdl::symbol_buffer<char>;
using symbol_interner = ovdl::symbol_interner<struct Id, char, std::size_t, void>;
using symbol = ovdl::symbol<char>;

namespace snitch {
	template<typename CharT>
	bool append(snitch::small_string_span ss, const ovdl::symbol<CharT>& s) {
		return append(ss, "{", static_cast<const void*>(s.c_str()), ",\"", s.view(), "\"}");
	}
}

TEST_CASE("symbol_buffer", "[symbol-buffer]") {
	static constexpr std::string_view buffer_in = "input value";
	static constexpr std::array<char, symbol_buffer::min_buffer_size - buffer_in.size()> fake_insert {};

	symbol_buffer buffer;

	{
		CAPTURE(buffer_in.size());
		CHECK(buffer.reserve(buffer_in.size()));
	}

	std::string_view buffer_val = buffer.insert(buffer_in.data(), buffer_in.size());
	CHECK(buffer_val == buffer_in);
	CHECK(std::distance(buffer.c_str(0), buffer_val.data() + buffer_val.size()) == buffer_in.size());

	// Minimum buffer size is 1024 * 16
	// The default buffer constructor is expected to treat this as the max size as well
	{
		CAPTURE(buffer.size());
		CAPTURE(fake_insert.size());
		CHECK_IF(buffer.reserve(buffer.size() + fake_insert.size())) {
			buffer.insert(fake_insert.data(), fake_insert.size() - 1);
		}
	}
	// Pinned vector buffer operates based on system page sizes
	// May have more capacity then specified
	// Ensure we attempt to reserve beyond vector's max size
	{
		CAPTURE(buffer.size());
		CAPTURE(ovdl::detail::pinned_vector<char>::page_size());
		CHECK_FALSE(buffer.reserve(buffer.size() + ovdl::detail::pinned_vector<char>::page_size()));
	}
}

TEST_CASE("symbol_buffer, max size", "[symbol-buffer-max-size]") {
	static constexpr std::string_view buffer_in = "input value";
	static constexpr std::array<char, symbol_buffer::min_buffer_size * 2 - buffer_in.size()> fake_insert {};

	symbol_buffer buffer(symbol_buffer::min_buffer_size * 2 + 1);

	{
		CAPTURE(buffer_in.size());
		CHECK(buffer.reserve(buffer_in.size()));
	}

	std::string_view buffer_val = buffer.insert(buffer_in.data(), buffer_in.size());
	CHECK(buffer_val == buffer_in);
	CHECK(std::distance(buffer.c_str(0), buffer_val.data() + buffer_val.size()) == buffer_in.size());

	{
		CAPTURE(buffer.size());
		CAPTURE(fake_insert.size());
		CHECK_IF(buffer.reserve(buffer.size() + fake_insert.size())) {
			buffer.insert(fake_insert.data(), fake_insert.size() - 1);
		}
	}
	// Pinned vector buffer operates based on system page sizes
	// May have more capacity then specified
	// Ensure we attempt to reserve beyond vector's max size
	{
		CAPTURE(buffer.size());
		CAPTURE(ovdl::detail::pinned_vector<char>::page_size());
		CHECK_FALSE(buffer.reserve(buffer.size() + ovdl::detail::pinned_vector<char>::page_size()));
	}
}

TEST_CASE("symbol_interner", "[symbol-intern]") {
	symbol_interner interner(symbol_buffer::min_buffer_size * 2);

	auto test = interner.intern("test");
	auto test2 = interner.intern("test");

	CHECK(test.view() == "test"sv);
	CHECK(test2.view() == "test"sv);

	CHECK(test == test2);

	auto test3 = interner.intern("test3");

	CHECK(test.view() == "test"sv);
	CHECK(test2.view() == "test"sv);
	CHECK(test3.view() == "test3"sv);

	CHECK(test == test2);
	CHECK_FALSE(test == test3);
	CHECK_FALSE(test2 == test3);

	CHECK_IF(interner.reserve(1024, 16 + 1)) {
		auto test4 = interner.intern("test3");

		CHECK(test.view() == "test"sv);
		CHECK(test2.view() == "test"sv);
		CHECK(test3.view() == "test3"sv);
		CHECK(test4.view() == "test3"sv);

		CHECK(test3 == test4);
		CHECK_FALSE(test == test3);
		CHECK_FALSE(test2 == test3);

		auto test5 = interner.intern("test5");

		CHECK(test.view() == "test"sv);
		CHECK(test2.view() == "test"sv);
		CHECK(test3.view() == "test3"sv);
		CHECK(test4.view() == "test3"sv);
		CHECK(test5.view() == "test5"sv);

		CHECK(test3 == test4);
		CHECK_FALSE(test == test3);
		CHECK_FALSE(test2 == test3);
		CHECK_FALSE(test5 == test);
		CHECK_FALSE(test5 == test2);
		CHECK_FALSE(test5 == test3);
		CHECK_FALSE(test5 == test4);
	}
}