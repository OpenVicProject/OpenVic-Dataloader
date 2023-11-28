#include <openvic-dataloader/File.hpp>

using namespace ovdl;

File::File(const char* path) : _path(path) {}

const char* File::path() const noexcept {
	return _path;
}