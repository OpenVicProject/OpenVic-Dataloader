#include <sstream>
#include <string_view>

#include <openvic-dataloader/csv/LineObject.hpp>

#include <range/v3/range/primitives.hpp>
#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace csv;
using namespace std::string_view_literals;

TEST_CASE("LineObject", "[line-object]") {
	LineObject line;

	SECTION("empty") {
		CHECK(ranges::size(line) == 0);

		CHECK(line.get_value_for(0) == ""sv);
		CHECK(line.get_value_for(1) == ""sv);
		CHECK(line.get_value_for(2) == ""sv);
		CHECK(line.get_value_for(3) == ""sv);
		CHECK(line.get_value_for(4) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line << std::flush;

			CHECK(ss.str() == ""sv);
		}
	}

	SECTION("no prefix") {
		line.push_back({ 0, "a" });
		line.push_back({ 1, "b" });
		line.push_back({ 2, "c" });
		line.set_suffix_end(3);

		CHECK(ranges::size(line) == 3);

		CHECK(line.get_value_for(0) == "a"sv);
		CHECK(line.get_value_for(1) == "b"sv);
		CHECK(line.get_value_for(2) == "c"sv);
		CHECK(line.get_value_for(3) == ""sv);
		CHECK(line.get_value_for(4) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line << std::flush;

			CHECK(ss.str() == "a;b;c"sv);
		}
	}

	SECTION("no suffix") {
		line.push_back({ 0, "a" });
		line.push_back({ 1, "b" });
		line.push_back({ 2, "c" });

		CHECK(ranges::size(line) == 3);

		CHECK_FALSE(line.get_value_for(0) == "a"sv);
		CHECK_FALSE(line.get_value_for(1) == "b"sv);
		CHECK_FALSE(line.get_value_for(2) == "c"sv);
		CHECK(line.get_value_for(3) == ""sv);
		CHECK(line.get_value_for(4) == ""sv);
	}

	SECTION("prefix and suffix") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a" });
		line.push_back({ 2, "b" });
		line.push_back({ 3, "c" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		CHECK(line.get_value_for(0) == ""sv);
		CHECK(line.get_value_for(1) == "a"sv);
		CHECK(line.get_value_for(2) == "b"sv);
		CHECK(line.get_value_for(3) == "c"sv);
		CHECK(line.get_value_for(4) == ""sv);
		CHECK(line.get_value_for(5) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line << std::flush;

			CHECK(ss.str() == ";a;b;c"sv);
		}
	}

	SECTION("prefix and suffix with spaces") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a b" });
		line.push_back({ 2, "c d" });
		line.push_back({ 3, "e f" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		CHECK(line.get_value_for(0) == ""sv);
		CHECK(line.get_value_for(1) == "a b"sv);
		CHECK(line.get_value_for(2) == "c d"sv);
		CHECK(line.get_value_for(3) == "e f"sv);
		CHECK(line.get_value_for(4) == ""sv);
		CHECK(line.get_value_for(5) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line << std::flush;

			CHECK(ss.str() == ";\"a b\";\"c d\";\"e f\""sv);
		}
	}

	SECTION("prefix and suffix with separators") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a;b" });
		line.push_back({ 2, "c;d" });
		line.push_back({ 3, "e;f" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		CHECK(line.get_value_for(0) == ""sv);
		CHECK(line.get_value_for(1) == "a;b"sv);
		CHECK(line.get_value_for(2) == "c;d"sv);
		CHECK(line.get_value_for(3) == "e;f"sv);
		CHECK(line.get_value_for(4) == ""sv);
		CHECK(line.get_value_for(5) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line << std::flush;

			CHECK(ss.str() == ";\"a;b\";\"c;d\";\"e;f\""sv);
		}
	}

	SECTION("prefix and suffix with custom char separator") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a;b" });
		line.push_back({ 2, "c;d" });
		line.push_back({ 3, "e;f" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		std::stringstream ss;
		ss << line.use_sep("|") << std::flush;

		CHECK(ss.str() == "|a;b|c;d|e;f"sv);
	}

	SECTION("prefix and suffix with custom char separator and containing the separator") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a|b" });
		line.push_back({ 2, "c|d" });
		line.push_back({ 3, "e|f" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		CHECK(line.get_value_for(0) == ""sv);
		CHECK(line.get_value_for(1) == "a|b"sv);
		CHECK(line.get_value_for(2) == "c|d"sv);
		CHECK(line.get_value_for(3) == "e|f"sv);
		CHECK(line.get_value_for(4) == ""sv);
		CHECK(line.get_value_for(5) == ""sv);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line.use_sep("|") << std::flush;

			CHECK(ss.str() == "|\"a|b\"|\"c|d\"|\"e|f\""sv);
		}
	}

	SECTION("prefix and suffix with custom string_view separator") {
		line.set_prefix_end(1);
		line.push_back({ 1, "a;b" });
		line.push_back({ 2, "c;d" });
		line.push_back({ 3, "e;f" });
		line.set_suffix_end(4);

		CHECK(ranges::size(line) == 3);

		SECTION("ostream print") {
			std::stringstream ss;
			ss << line.use_sep("hey") << std::flush;

			CHECK(ss.str() == "heya;bheyc;dheye;f"sv);
		}
	}
}