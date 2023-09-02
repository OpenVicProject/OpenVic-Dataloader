#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

#include <openvic-dataloader/v2script/Parser.hpp>

int main(int argc, char** argv) {
	if (argc < 2) {
		std::fprintf(stderr, "usage: %s <filename>", argv[0]);
		return 1;
	}

	auto parser = ovdl::v2script::Parser::from_file(argv[1]);
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

	return 0;
}