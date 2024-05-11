#include <filesystem>
#include <iostream>
#include <ostream>

#include <openvic-dataloader/Parser.hpp>

#include "detail/NullBuff.hpp"

using namespace ovdl;
using namespace ovdl::detail;

BasicParser::BasicParser() : _error_stream(detail::cnull) {}

void BasicParser::set_error_log_to_null() {
	set_error_log_to(detail::cnull);
}

void BasicParser::set_error_log_to_stderr() {
	set_error_log_to(std::cerr);
}

void BasicParser::set_error_log_to_stdout() {
	set_error_log_to(std::cout);
}

void BasicParser::set_error_log_to(std::basic_ostream<char>& stream) {
	_error_stream = stream;
}

bool BasicParser::has_error() const {
	return _has_error;
}

bool BasicParser::has_fatal_error() const {
	return _has_fatal_error;
}

bool BasicParser::has_warning() const {
	return _has_warning;
}

std::string_view BasicParser::get_file_path() const {
	return _file_path;
}

void BasicParser::set_file_path(std::string_view path) {
	std::error_code error;
	std::filesystem::path file_path = std::filesystem::weakly_canonical(path, error);
	_file_path = file_path.string();
}