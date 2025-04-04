#include "File.hpp"

#include <cstring>

#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/encoding.hpp>

using namespace ovdl;

File::File(const char* path) : _path(path) {}

const char* File::path() const noexcept {
	return _path;
}

bool File::is_valid() const noexcept {
	return _buffer.data() != nullptr;
}

std::size_t File::size() const noexcept {
	return _buffer.size();
}