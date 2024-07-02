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
	return _buffer.index() != 0 && !_buffer.valueless_by_exception() && visit_buffer([](auto&& buffer) { return buffer.data() != nullptr; });
}

std::size_t File::size() const noexcept {
	return _buffer.index() != 0 && !_buffer.valueless_by_exception() ? _buffer_size : 0;
}