#include <string_view>

#include <openvic-dataloader/NodeLocation.hpp>

#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace std::string_view_literals;

TEST_CASE("NodeLocation", "[node-location]") {
	BasicNodeLocation<char> char_node_location;
	CHECK(char_node_location.begin() == nullptr);
	CHECK(char_node_location.end() == nullptr);
	CHECK(char_node_location.is_synthesized());

	BasicNodeLocation<unsigned char> uchar_node_location;
	CHECK(uchar_node_location.begin() == nullptr);
	CHECK(uchar_node_location.end() == nullptr);
	CHECK(uchar_node_location.is_synthesized());

	BasicNodeLocation<char16_t> char_16_node_location;
	CHECK(char_16_node_location.begin() == nullptr);
	CHECK(char_16_node_location.end() == nullptr);
	CHECK(char_16_node_location.is_synthesized());

	BasicNodeLocation<char32_t> char_32_node_location;
	CHECK(char_32_node_location.begin() == nullptr);
	CHECK(char_32_node_location.end() == nullptr);
	CHECK(char_32_node_location.is_synthesized());

#ifdef __cpp_char8_t
	BasicNodeLocation<char8_t> char_8_node_location;
	CHECK(char_8_node_location.begin() == nullptr);
	CHECK(char_8_node_location.end() == nullptr);
	CHECK(char_8_node_location.is_synthesized());
#endif

	static constexpr auto char_buffer = "buffer"sv;

	static constexpr unsigned char uarray[] = "buffer";
	static constexpr std::basic_string_view<unsigned char> uchar_buffer = uarray;

	static constexpr auto char_16_buffer = u"buffer"sv;
	static constexpr auto char_32_buffer = U"buffer"sv;

#ifdef __cpp_char8_t
	static constexpr auto char_8_buffer = u8"buffer"sv;
#endif

	char_node_location = { &char_buffer[0] };
	CHECK(char_node_location.begin() == &char_buffer[0]);
	CHECK(char_node_location.end() == &char_buffer[0]);
	CHECK_FALSE(char_node_location.is_synthesized());

	uchar_node_location = { &uchar_buffer[0] };
	CHECK(uchar_node_location.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_location.end() == &uchar_buffer[0]);
	CHECK_FALSE(uchar_node_location.is_synthesized());

	char_16_node_location = { &char_16_buffer[0] };
	CHECK(char_16_node_location.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_location.end() == &char_16_buffer[0]);
	CHECK_FALSE(char_16_node_location.is_synthesized());

	char_32_node_location = { &char_32_buffer[0] };
	CHECK(char_32_node_location.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_location.end() == &char_32_buffer[0]);
	CHECK_FALSE(char_32_node_location.is_synthesized());

#ifdef __cpp_char8_t
	char_8_node_location = { &char_8_buffer[0] };
	CHECK(char_8_node_location.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_location.end() == &char_8_buffer[0]);
	CHECK_FALSE(char_8_node_location.is_synthesized());
#endif

	char_node_location = { &char_buffer[0], &char_buffer.back() };
	CHECK(char_node_location.begin() == &char_buffer[0]);
	CHECK(char_node_location.end() == &char_buffer.back());
	CHECK_FALSE(char_node_location.is_synthesized());

	uchar_node_location = { &uchar_buffer[0], &uchar_buffer.back() };
	CHECK(uchar_node_location.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_location.end() == &uchar_buffer.back());
	CHECK_FALSE(uchar_node_location.is_synthesized());

	char_16_node_location = { &char_16_buffer[0], &char_16_buffer.back() };
	CHECK(char_16_node_location.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_location.end() == &char_16_buffer.back());
	CHECK_FALSE(char_16_node_location.is_synthesized());

	char_32_node_location = { &char_32_buffer[0], &char_32_buffer.back() };
	CHECK(char_32_node_location.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_location.end() == &char_32_buffer.back());
	CHECK_FALSE(char_32_node_location.is_synthesized());

#ifdef __cpp_char8_t
	char_8_node_location = { &char_8_buffer[0], &char_8_buffer.back() };
	CHECK(char_8_node_location.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_location.end() == &char_8_buffer.back());
	CHECK_FALSE(char_8_node_location.is_synthesized());
#endif

	BasicNodeLocation<char> char_node_location_copy = { char_node_location };
	char_node_location_copy._begin++;
	CHECK(char_node_location.begin() == &char_buffer[0]);
	CHECK(char_node_location_copy.begin() == &char_buffer[1]);
	CHECK(char_node_location_copy.end() == &char_buffer.back());
	CHECK_FALSE(char_node_location_copy.is_synthesized());

	BasicNodeLocation<unsigned char> uchar_node_location_copy = { uchar_node_location };
	uchar_node_location_copy._begin++;
	CHECK(uchar_node_location.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_location_copy.begin() == &uchar_buffer[1]);
	CHECK(uchar_node_location_copy.end() == &uchar_buffer.back());
	CHECK_FALSE(uchar_node_location_copy.is_synthesized());

	BasicNodeLocation<char16_t> char_16_node_location_copy = { char_16_node_location };
	char_16_node_location_copy._begin++;
	CHECK(char_16_node_location.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_location_copy.begin() == &char_16_buffer[1]);
	CHECK(char_16_node_location_copy.end() == &char_16_buffer.back());
	CHECK_FALSE(char_16_node_location_copy.is_synthesized());

	BasicNodeLocation<char32_t> char_32_node_location_copy = { char_32_node_location };
	char_32_node_location_copy._begin++;
	CHECK(char_32_node_location.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_location_copy.begin() == &char_32_buffer[1]);
	CHECK(char_32_node_location_copy.end() == &char_32_buffer.back());
	CHECK_FALSE(char_32_node_location_copy.is_synthesized());

#ifdef __cpp_char8_t
	BasicNodeLocation<char8_t> char_8_node_location_copy = { char_8_node_location };
	char_8_node_location_copy._begin++;
	CHECK(char_8_node_location.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_location_copy.begin() == &char_8_buffer[1]);
	CHECK(char_8_node_location_copy.end() == &char_8_buffer.back());
	CHECK_FALSE(char_8_node_location_copy.is_synthesized());
#endif

	BasicNodeLocation<char> char_node_location_move = { std::move(char_node_location) };
	CHECK(char_node_location_move.begin() == char_node_location.begin());
	CHECK_FALSE(char_node_location.is_synthesized());
	CHECK(char_node_location_move.begin() == &char_buffer[0]);
	CHECK(char_node_location_move.end() == &char_buffer.back());
	CHECK_FALSE(char_node_location_move.is_synthesized());

	BasicNodeLocation<unsigned char> uchar_node_location_move = { std::move(uchar_node_location) };
	CHECK(uchar_node_location_move.begin() == uchar_node_location.begin());
	CHECK_FALSE(uchar_node_location.is_synthesized());
	CHECK(uchar_node_location_move.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_location_move.end() == &uchar_buffer.back());
	CHECK_FALSE(uchar_node_location_move.is_synthesized());

	BasicNodeLocation<char16_t> char_16_node_location_move = { std::move(char_16_node_location) };
	CHECK(char_16_node_location_move.begin() == char_16_node_location.begin());
	CHECK_FALSE(char_16_node_location.is_synthesized());
	CHECK(char_16_node_location_move.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_location_move.end() == &char_16_buffer.back());
	CHECK_FALSE(char_16_node_location_move.is_synthesized());

	BasicNodeLocation<char32_t> char_32_node_location_move = { std::move(char_32_node_location) };
	CHECK(char_32_node_location_move.begin() == char_32_node_location.begin());
	CHECK_FALSE(char_32_node_location_move.is_synthesized());
	CHECK(char_32_node_location_move.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_location_move.end() == &char_32_buffer.back());
	CHECK_FALSE(char_32_node_location_move.is_synthesized());

#ifdef __cpp_char8_t
	BasicNodeLocation<char8_t> char_8_node_location_move = { std::move(char_8_node_location) };
	CHECK(char_8_node_location_move.begin() == char_8_node_location.begin());
	CHECK_FALSE(char_8_node_location.is_synthesized());
	CHECK(char_8_node_location_move.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_location_move.end() == &char_8_buffer.back());
	CHECK_FALSE(char_8_node_location_move.is_synthesized());
#endif

	BasicNodeLocation<char> char_node_from;
	char_node_from.set_from(char_node_location);
	CHECK(char_node_from.begin() == &char_buffer[0]);
	CHECK(char_node_from.end() == &char_buffer.back());
	CHECK_FALSE(char_node_from.is_synthesized());

	BasicNodeLocation<unsigned char> uchar_node_from;
	uchar_node_from.set_from(uchar_node_location);
	CHECK(uchar_node_from.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_from.end() == &uchar_buffer.back());
	CHECK_FALSE(uchar_node_from.is_synthesized());

	BasicNodeLocation<char16_t> char_16_node_from;
	char_16_node_from.set_from(char_16_node_location);
	CHECK(char_16_node_from.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_from.end() == &char_16_buffer.back());
	CHECK_FALSE(char_16_node_from.is_synthesized());

	BasicNodeLocation<char32_t> char_32_node_from;
	char_32_node_from.set_from(char_32_node_location);
	CHECK(char_32_node_from.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_from.end() == &char_32_buffer.back());
	CHECK_FALSE(char_32_node_from.is_synthesized());

#ifdef __cpp_char8_t
	BasicNodeLocation<char8_t> char_8_node_from;
	char_8_node_from.set_from(char_8_node_location);
	CHECK(char_8_node_from.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_from.end() == &char_8_buffer.back());
	CHECK_FALSE(char_8_node_from.is_synthesized());
#endif

	char_node_from.set_from(uchar_node_location);
	CHECK(char_node_from.begin() == reinterpret_cast<const char*>(&uchar_buffer[0]));
	CHECK(char_node_from.end() == reinterpret_cast<const char*>(&uchar_buffer.back()));
	CHECK_FALSE(char_node_from.is_synthesized());

	uchar_node_from.set_from(char_node_location);
	CHECK(uchar_node_from.begin() == reinterpret_cast<const unsigned char*>(&char_buffer[0]));
	CHECK(uchar_node_from.end() == reinterpret_cast<const unsigned char*>(&char_buffer.back()));
	CHECK_FALSE(uchar_node_from.is_synthesized());

	char_16_node_from.set_from(char_node_location);
	CHECK(char_16_node_from.begin() == reinterpret_cast<const char16_t*>(&char_buffer[0]));
	CHECK(char_16_node_from.end() == reinterpret_cast<const char16_t*>(&char_buffer.back() - 1));
	CHECK_FALSE(char_16_node_from.is_synthesized());

	char_32_node_from.set_from(char_node_location);
	CHECK(char_32_node_from.begin() == reinterpret_cast<const char32_t*>(&char_buffer[0]));
	CHECK(char_32_node_from.end() == reinterpret_cast<const char32_t*>(&char_buffer.back() - 3));
	CHECK_FALSE(char_32_node_from.is_synthesized());

#ifdef __cpp_char8_t
	char_8_node_from.set_from(char_node_location);
	CHECK(char_8_node_from.begin() == reinterpret_cast<const char8_t*>(&char_buffer[0]));
	CHECK(char_8_node_from.end() == reinterpret_cast<const char8_t*>(&char_buffer.back()));
	CHECK_FALSE(char_8_node_from.is_synthesized());
#endif

	char_node_from.set_from(char_16_node_location);
	CHECK(char_node_from.begin() == reinterpret_cast<const char*>(&char_16_buffer[0]));
	CHECK(char_node_from.end() == reinterpret_cast<const char*>(&char_16_buffer.back()) + 1);
	CHECK_FALSE(char_node_from.is_synthesized());

	char_node_from.set_from(char_32_node_location);
	CHECK(char_node_from.begin() == reinterpret_cast<const char*>(&char_32_buffer[0]));
	CHECK(char_node_from.end() == reinterpret_cast<const char*>(&char_32_buffer.back()) + 3);
	CHECK_FALSE(char_node_from.is_synthesized());

	auto char_node_make = BasicNodeLocation<char>::make_from(&char_buffer[0], &char_buffer.back());
	CHECK(char_node_make.begin() == &char_buffer[0]);
	CHECK(char_node_make.end() == &char_buffer.back() + 1);
	CHECK_FALSE(char_node_make.is_synthesized());

	auto uchar_node_make = BasicNodeLocation<unsigned char>::make_from(&uchar_buffer[0], &uchar_buffer.back());
	CHECK(uchar_node_make.begin() == &uchar_buffer[0]);
	CHECK(uchar_node_make.end() == &uchar_buffer.back() + 1);
	CHECK_FALSE(uchar_node_make.is_synthesized());

	auto char_16_node_make = BasicNodeLocation<char16_t>::make_from(&char_16_buffer[0], &char_16_buffer.back());
	CHECK(char_16_node_make.begin() == &char_16_buffer[0]);
	CHECK(char_16_node_make.end() == &char_16_buffer.back() + 1);
	CHECK_FALSE(char_16_node_make.is_synthesized());

	auto char_32_node_make = BasicNodeLocation<char32_t>::make_from(&char_32_buffer[0], &char_32_buffer.back());
	CHECK(char_32_node_make.begin() == &char_32_buffer[0]);
	CHECK(char_32_node_make.end() == &char_32_buffer.back() + 1);
	CHECK_FALSE(char_32_node_make.is_synthesized());

#ifdef __cpp_char8_t
	auto char_8_node_make = BasicNodeLocation<char8_t>::make_from(&char_8_buffer[0], &char_8_buffer.back());
	CHECK(char_8_node_make.begin() == &char_8_buffer[0]);
	CHECK(char_8_node_make.end() == &char_8_buffer.back() + 1);
	CHECK_FALSE(char_8_node_make.is_synthesized());
#endif

	char_node_make = BasicNodeLocation<char>::make_from(&char_buffer[0] + 1, &char_buffer[0]);
	CHECK(char_node_make.begin() == &char_buffer[0] + 1);
	CHECK(char_node_make.end() == &char_buffer[0] + 1);
	CHECK_FALSE(char_node_make.is_synthesized());

	uchar_node_make = BasicNodeLocation<unsigned char>::make_from(&uchar_buffer[0] + 1, &uchar_buffer[0]);
	CHECK(uchar_node_make.begin() == &uchar_buffer[0] + 1);
	CHECK(uchar_node_make.end() == &uchar_buffer[0] + 1);
	CHECK_FALSE(uchar_node_make.is_synthesized());

	char_16_node_make = BasicNodeLocation<char16_t>::make_from(&char_16_buffer[0] + 1, &char_16_buffer[0]);
	CHECK(char_16_node_make.begin() == &char_16_buffer[0] + 1);
	CHECK(char_16_node_make.end() == &char_16_buffer[0] + 1);
	CHECK_FALSE(char_16_node_make.is_synthesized());

	char_32_node_make = BasicNodeLocation<char32_t>::make_from(&char_32_buffer[0] + 1, &char_32_buffer[0]);
	CHECK(char_32_node_make.begin() == &char_32_buffer[0] + 1);
	CHECK(char_32_node_make.end() == &char_32_buffer[0] + 1);
	CHECK_FALSE(char_32_node_make.is_synthesized());

#ifdef __cpp_char8_t
	char_8_node_make = BasicNodeLocation<char8_t>::make_from(&char_8_buffer[0] + 1, &char_8_buffer[0]);
	CHECK(char_8_node_make.begin() == &char_8_buffer[0] + 1);
	CHECK(char_8_node_make.end() == &char_8_buffer[0] + 1);
	CHECK_FALSE(char_8_node_make.is_synthesized());
#endif
}