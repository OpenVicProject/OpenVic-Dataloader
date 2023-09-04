#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

std::string_view trim(std::string_view str) {
	std::string_view::iterator begin = str.begin();
	std::string_view::iterator end = str.end();
	for (;; begin++) {
		if (begin == end) return std::string_view();
		if (!std::isspace(*begin)) break;
	}
	end--;
	for (;; end--) {
		if (end == begin) return std::string_view();
		if (!std::isspace(*end)) break;
	}
	return std::string_view(&*begin, std::distance(begin, end));
}

bool insenitive_trim_eq(std::string_view lhs, std::string_view rhs) {
	lhs = trim(lhs);
	rhs = trim(rhs);
	return std::equal(
		lhs.begin(), lhs.end(),
		rhs.begin(), rhs.end(),
		[](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

template<ovdl::csv::EncodingType Encoding>
int print_csv(const std::string_view path) {
	auto parser = ovdl::csv::Parser<Encoding>::from_file(path);
	if (parser.has_error()) {
		return 1;
	}

	parser.parse_csv();
	if (parser.has_error()) {
		return 2;
	}

	if (parser.has_warning()) {
		for (auto& warning : parser.get_warnings()) {
			std::cerr << "Warning: " << warning.message << std::endl;
		}
	}

	std::cout << "lines:\t\t" << parser.get_lines().size() << std::endl;
	for (const auto& line : parser.get_lines()) {
		std::cout << "line size:\t" << line.value_count() << std::endl;
		std::cout << "values:\t\t" << line << std::endl;
	}
	return EXIT_SUCCESS;
}

int print_v2script_simple(const std::string_view path) {
	auto parser = ovdl::v2script::Parser::from_file(path);
	if (parser.has_error()) {
		return 1;
	}

	parser.simple_parse();
	if (parser.has_error()) {
		return 2;
	}

	if (parser.has_warning()) {
		for (auto& warning : parser.get_warnings()) {
			std::cerr << "Warning: " << warning.message << std::endl;
		}
	}

	std::cout << parser.get_file_node() << std::endl;
	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	switch (argc) {
		case 2:
			return print_v2script_simple(argv[1]);
		case 4:
			if (insenitive_trim_eq(argv[1], "csv") && insenitive_trim_eq(argv[2], "utf"))
				return print_csv<ovdl::csv::EncodingType::Utf8>(argv[3]);
			goto default_jump;
		case 3:
			if (insenitive_trim_eq(argv[1], "csv"))
				return print_csv<ovdl::csv::EncodingType::Windows1252>(argv[2]);
			[[fallthrough]];
		default:
		default_jump:
			std::fprintf(stderr, "usage: %s <filename>\n", argv[0]);
			std::fprintf(stderr, "usage: %s csv <filename>\n", argv[0]);
			std::fprintf(stderr, "usage: %s csv utf <filename>", argv[0]);
			return EXIT_FAILURE;
	}

	return 0;
}