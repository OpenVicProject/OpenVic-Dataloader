#include <filesystem>
#include <fstream>
#include <string_view>

#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>

#include <fmt/core.h>

#include "Helper.hpp"
#include <detail/NullBuff.hpp>
#include <range/v3/range/primitives.hpp>
#include <range/v3/view/enumerate.hpp>
#include <range/v3/view/iota.hpp>
#include <range/v3/view/join.hpp>
#include <snitch/snitch.hpp>

using namespace ovdl;
using namespace csv;
using namespace std::string_view_literals;

static constexpr auto csv_buffer = "a;b;c"sv;
static constexpr auto csv_path = "file.csv"sv;

static void SetupFile(std::string_view path) {
	std::ofstream stream(path.data());
	stream << csv_buffer << std::flush;
}

#define CHECK_PARSE(...)                            \
	CHECK_OR_RETURN(parser.get_errors().empty());   \
	CHECK_OR_RETURN(parser.parse_csv(__VA_ARGS__)); \
	CHECK_OR_RETURN(parser.get_errors().empty());

TEST_CASE("CSV Memory Buffer (data, size) Parse", "[csv-memory-parse][buffer][data-size]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(csv_buffer.data(), csv_buffer.size());

	CHECK_PARSE(false);
}

TEST_CASE("CSV Memory Buffer (begin, end) Parse", "[csv-memory-parse][buffer][begin-end]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(csv_buffer.data(), csv_buffer.data() + csv_buffer.size());

	CHECK_PARSE(false);
}

TEST_CASE("CSV Buffer nullptr Parse", "[csv-memory-parse][buffer][nullptr]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(nullptr, std::size_t { 0 });

	CHECK_PARSE(true);
}

TEST_CASE("CSV Memory String Parse", "[csv-memory-parse][string]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_string(csv_buffer);

	CHECK_PARSE(true);
}

TEST_CASE("CSV Memory Buffer (data, size) Handle String Parse", "[csv-memory-parse][handle-string][buffer][data-size]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(csv_buffer.data(), csv_buffer.size());

	CHECK_PARSE(true);
}

TEST_CASE("CSV Memory Buffer (begin, end) Handle String Parse", "[csv-memory-parse][handle-string][buffer][begin-end]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_buffer(csv_buffer.data(), csv_buffer.data() + csv_buffer.size());

	CHECK_PARSE(false);
}

TEST_CASE("CSV Memory String Handle String Parse", "[csv-memory-parse][handle-string][string]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_string(csv_buffer);

	CHECK_PARSE(true);
}

TEST_CASE("CSV File (const char*) Parse", "[csv-file-parse][char-ptr]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(csv_path.data());

	std::filesystem::remove(csv_path);

	CHECK_PARSE(false);
}

TEST_CASE("CSV File (filesystem::path) Parse", "[csv-file-parse][filesystem-path]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::filesystem::path(csv_path));

	std::filesystem::remove(csv_path);

	CHECK_PARSE(false);
}

TEST_CASE("CSV File (HasCstr) Parse", "[csv-file-parse][has-cstr]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::string { csv_path });

	std::filesystem::remove(csv_path);

	CHECK_PARSE(false);
}

TEST_CASE("CSV File (const char*) Handle String Parse", "[csv-file-parse][handle-string][char-ptr]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(csv_path.data());

	std::filesystem::remove(csv_path);

	CHECK_PARSE(true);
}

TEST_CASE("CSV File (filesystem::path) Handle String Parse", "[csv-file-parse][handle-string][filesystem-path]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::filesystem::path(csv_path));

	std::filesystem::remove(csv_path);

	CHECK_PARSE(true);
}

TEST_CASE("CSV File (HasCstr) Handle String Parse", "[csv-file-parse][handle-string][has-cstr]") {
	SetupFile(csv_path);

	Parser parser(ovdl::detail::cnull);

	parser.load_from_file(std::string { csv_path });

	std::filesystem::remove(csv_path);

	CHECK_PARSE(true);
}

TEST_CASE("CSV File (const char*) Handle Empty Path String Parse", "[csv-file-parse][handle-string][char-ptr][empty-path]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_file("");

	CHECK_OR_RETURN(!parser.get_errors().empty());
}

TEST_CASE("CSV File (const char*) Handle Non-existent Path String Parse", "[csv-file-parse][handle-string][char-ptr][nonexistent-path]") {
	Parser parser(ovdl::detail::cnull);

	parser.load_from_file("./Idontexist");

	CHECK_OR_RETURN(!parser.get_errors().empty());
}

TEST_CASE("CSV Parse", "[csv-parse]") {
	Parser parser(ovdl::detail::cnull);

	SECTION("a;b;c") {
		static constexpr auto buffer = "a;b;c"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 3);
		CHECK(line.prefix_end() == 0);
		CHECK(line.suffix_end() == 3);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 0);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 3);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";a;b;c") {
		static constexpr auto buffer = ";a;b;c"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 4);
		CHECK(line.prefix_end() == 1);
		CHECK(line.suffix_end() == 4);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 4);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";a;b;c;") {
		static constexpr auto buffer = ";a;b;c;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 4);
		CHECK(line.prefix_end() == 1);
		CHECK(line.suffix_end() == 4);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 4);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";;a;b;c;") {
		static constexpr auto buffer = ";;a;b;c;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 5);
		CHECK(line.prefix_end() == 2);
		CHECK(line.suffix_end() == 5);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 4);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 5);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0:
				case 1: break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 4: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";;a;b;c;;") {
		static constexpr auto buffer = ";;a;b;c;;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 6);
		CHECK(line.prefix_end() == 2);
		CHECK(line.suffix_end() == 6);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 4);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 6);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0:
				case 1:
				case 5: break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 4: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";;a;b;;c;;") {
		static constexpr auto buffer = ";;a;b;;c;;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 7);
		CHECK(line.prefix_end() == 2);
		CHECK(line.suffix_end() == 7);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 5);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 7);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0:
				case 1:
				case 4:
				case 6: break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 5: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";;a;b;;c;;\\n;d;e;;f;g;;") {
		static constexpr auto buffer = ";;a;b;;c;;\n;d;e;;f;g;;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 2);

		auto it = line_list.begin();

		const LineObject& line = *it;
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 7);
		CHECK(line.prefix_end() == 2);
		CHECK(line.suffix_end() == 7);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "a"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "b"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 5);
					CHECK_OR_CONTINUE(val.second == "c"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 7);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0:
				case 1:
				case 4:
				case 6: break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "a"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "b"sv); break;
				case 5: CHECK_OR_CONTINUE(line.get_value_for(index) == "c"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		it++;
		CHECK(it != line_list.end());

		const LineObject& line2 = *it;
		CHECK_FALSE(line2.empty());
		CHECK(ranges::size(line2) == 4);
		CHECK(line2.value_count() == 7);
		CHECK(line2.prefix_end() == 1);
		CHECK(line2.suffix_end() == 7);

		for (const auto [index, val] : line2 | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "d"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "e"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 4);
					CHECK_OR_CONTINUE(val.second == "f"sv);
					break;
				case 3:
					CHECK_OR_CONTINUE(val.first == 5);
					CHECK_OR_CONTINUE(val.second == "g"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line2.value_count() == 7);

		for (const auto index : ranges::views::iota(size_t(0), line2.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0:
				case 3:
				case 6: break;
				case 1: CHECK_OR_CONTINUE(line2.get_value_for(index) == "d"sv); break;
				case 2: CHECK_OR_CONTINUE(line2.get_value_for(index) == "e"sv); break;
				case 4: CHECK_OR_CONTINUE(line2.get_value_for(index) == "f"sv); break;
				case 5: CHECK_OR_CONTINUE(line2.get_value_for(index) == "g"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION("Score militaire;Militär;;Puntuación militar") {
		static constexpr auto buffer = "Score militaire;Militär;;Puntuación militar"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 3);
		CHECK(line.value_count() == 4);
		CHECK(line.prefix_end() == 0);
		CHECK(line.suffix_end() == 4);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 0);
					CHECK_OR_CONTINUE(val.second == "Score militaire"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "Militär"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == "Puntuación militar"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 4);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: CHECK_OR_CONTINUE(line.get_value_for(index) == "Score militaire"sv); break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "Militär"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == ""sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == "Puntuación militar"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION(";§RNo research set§W;§RAucune recherche définie§W;") {
		static constexpr auto buffer = ";§RNo research set§W;§RAucune recherche définie§W;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 2);
		CHECK(line.value_count() == 3);
		CHECK(line.prefix_end() == 1);
		CHECK(line.suffix_end() == 3);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "§RNo research set§W"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "§RAucune recherche définie§W"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 3);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: CHECK_OR_CONTINUE(line.get_value_for(index) == ""sv); break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "§RNo research set§W"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "§RAucune recherche définie§W"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	SECTION("Württemberg;Wurtemberg;Württemberg;;Württemberg;") {
		static constexpr auto buffer = "Württemberg;Wurtemberg;Württemberg;;Württemberg;"sv;
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 4);
		CHECK(line.value_count() == 5);
		CHECK(line.prefix_end() == 0);
		CHECK(line.suffix_end() == 5);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 0);
					CHECK_OR_CONTINUE(val.second == "Württemberg"sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "Wurtemberg"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "Württemberg"sv);
					break;
				case 3:
					CHECK_OR_CONTINUE(val.first == 4);
					CHECK_OR_CONTINUE(val.second == "Württemberg"sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 5);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: CHECK_OR_CONTINUE(line.get_value_for(index) == "Württemberg"sv); break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "Wurtemberg"sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "Württemberg"sv); break;
				case 3: CHECK_OR_CONTINUE(line.get_value_for(index) == ""sv); break;
				case 4: CHECK_OR_CONTINUE(line.get_value_for(index) == "Württemberg"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}

	// Blame Ubuntu 22's GCC-12 distribution for this crap
	// Compiler bug hangs if it can see if there is any reference to \x8F in a character
#if !defined(_OVDL_TEST_UBUNTU_GCC_12_BUG_)
	SECTION(";$NAME$ wurde in $PROV$ gebaut.;ID'\\x8F' DO;") {
		static auto buffer = ";$NAME$ wurde in $PROV$ gebaut.;ID\x8F DO;";
		parser.load_from_string(buffer);

		CHECK_PARSE();

		const std::vector<LineObject>& line_list = parser.get_lines();
		CHECK_FALSE(line_list.empty());
		CHECK(ranges::size(line_list) == 1);

		const LineObject& line = line_list.front();
		CHECK_FALSE(line.empty());
		CHECK(ranges::size(line) == 2);
		CHECK(line.value_count() == 3);
		CHECK(line.prefix_end() == 1);
		CHECK(line.suffix_end() == 3);

		for (const auto [index, val] : line | ranges::views::enumerate) {
			CAPTURE(index);
			CHECK_FALSE_OR_CONTINUE(val.second.empty());
			switch (index) {
				case 0:
					CHECK_OR_CONTINUE(val.first == 1);
					CHECK_OR_CONTINUE(val.second == "$NAME$ wurde in $PROV$ gebaut."sv);
					break;
				case 1:
					CHECK_OR_CONTINUE(val.first == 2);
					CHECK_OR_CONTINUE(val.second == "IDĘ DO"sv);
					break;
				case 2:
					CHECK_OR_CONTINUE(val.first == 3);
					CHECK_OR_CONTINUE(val.second == ""sv);
					break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}

		CHECK(line.value_count() == 3);

		for (const auto index : ranges::views::iota(size_t(0), line.value_count())) {
			CAPTURE(index);
			switch (index) {
				case 0: CHECK_OR_CONTINUE(line.get_value_for(index) == ""sv); break;
				case 1: CHECK_OR_CONTINUE(line.get_value_for(index) == "$NAME$ wurde in $PROV$ gebaut."sv); break;
				case 2: CHECK_OR_CONTINUE(line.get_value_for(index) == "IDĘ DO"sv); break;
				default: CHECK_OR_CONTINUE(false); break;
			}
		}
	}
#endif
}