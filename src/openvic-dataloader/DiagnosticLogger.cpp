#include <openvic-dataloader/DiagnosticLogger.hpp>

using namespace ovdl;

DiagnosticLogger::operator bool() const {
	return !_errored;
}

bool DiagnosticLogger::errored() const { return _errored; }
bool DiagnosticLogger::warned() const { return _warned; }


NodeLocation DiagnosticLogger::location_of(const error::Error* error) const {
	auto result = _map.lookup(error);
	return result ? *result : NodeLocation{};
}