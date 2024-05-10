#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <string_view>

#include <openvic-dataloader/ParseWarning.hpp>
#include <openvic-dataloader/csv/LineObject.hpp>
#include <openvic-dataloader/csv/Parser.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/v2script/Parser.hpp>

enum class VisualizationType {
	Native, // <name> = { <contents> }
	List	// - <type> : <multiline contents>
};

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
	auto parser = ovdl::csv::Parser<Encoding>(std::cerr);
	parser.load_from_file(path);
	if (parser.has_error()) {
		return 1;
	}

	parser.parse_csv();
	if (parser.has_error()) {
		return 2;
	}

	if (parser.has_warning()) {
		parser.print_errors_to(std::cerr);
	}

	std::cout << "lines:\t\t" << parser.get_lines().size() << std::endl;
	for (const auto& line : parser.get_lines()) {
		std::cout << "line size:\t" << line.value_count() << std::endl;
		std::cout << "values:\t\t" << line << std::endl;
	}
	return EXIT_SUCCESS;
}

int print_lua(const std::string_view path, VisualizationType visual_type) {
	auto parser = ovdl::v2script::Parser(std::cerr);
	parser.load_from_file(path);
	if (parser.has_error()) {
		return 1;
	}

	parser.lua_defines_parse();
	if (parser.has_error()) {
		return 2;
	}

	if (parser.has_warning()) {
		parser.print_errors_to(std::cerr);
	}

	switch (visual_type) {
		using enum VisualizationType;
		case Native: std::cout << parser.make_native_string() << '\n'; break;
		case List: std::cout << parser.make_list_string() << '\n'; break;
	}
	return EXIT_SUCCESS;
}

int print_v2script_simple(const std::string_view path, VisualizationType visual_type) {
	auto parser = ovdl::v2script::Parser(std::cerr);
	parser.load_from_file(path);
	if (parser.has_error()) {
		return 1;
	}

	parser.simple_parse();
	if (parser.has_error()) {
		return 2;
	}

	if (parser.has_warning()) {
		parser.print_errors_to(std::cerr);
	}

	switch (visual_type) {
		using enum VisualizationType;
		case Native: std::cout << parser.make_native_string() << '\n'; break;
		case List: std::cout << parser.make_list_string() << '\n'; break;
	}
	return EXIT_SUCCESS;
}

int main(int argc, char** argv) {
	std::vector<std::string> args;
	args.reserve(argc);
	for (size_t index = 0; index < argc; index++) {
		args.push_back(argv[index]);
	}

	VisualizationType type = VisualizationType::Native;
	if (args.size() >= 2) {
		std::string_view type_str = args[1];
		if (insenitive_trim_eq(type_str, "list")) {
			type = VisualizationType::List;
			args.erase(args.begin() + 1);
		} else if (insenitive_trim_eq(type_str, "native")) {
			type = VisualizationType::Native;
			args.erase(args.begin() + 1);
		}
	}

	switch (args.size()) {
		case 2:
			if (insenitive_trim_eq(std::filesystem::path(args[1]).extension().string(), ".lua")) {
				return print_lua(args[1], type);
			}
			return print_v2script_simple(args[1], type);
		case 4:
			if (insenitive_trim_eq(args[1], "csv") && insenitive_trim_eq(args[2], "utf"))
				return print_csv<ovdl::csv::EncodingType::Utf8>(args[3]);
			goto default_jump;
		case 3:
			if (insenitive_trim_eq(args[1], "csv"))
				return print_csv<ovdl::csv::EncodingType::Windows1252>(args[2]);
			if (insenitive_trim_eq(args[1], "lua"))
				return print_lua(args[2], type);
			[[fallthrough]];
		default:
		default_jump:
			std::fprintf(stderr, "usage: %s <filename>\n", args[0].c_str());
			std::fprintf(stderr, "usage: %s list <options> <filename>\n", args[0].c_str());
			std::fprintf(stderr, "usage: %s native <options> <filename>\n", args[0].c_str());
			std::fprintf(stderr, "usage: %s lua <filename>\n", args[0].c_str());
			std::fprintf(stderr, "usage: %s csv [utf] <filename>\n", args[0].c_str());
			return EXIT_FAILURE;
	}

	return 0;
}