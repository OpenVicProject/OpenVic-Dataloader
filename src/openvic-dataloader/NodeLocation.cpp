#include <openvic-dataloader/NodeLocation.hpp>

using namespace ovdl;

NodeLocation::NodeLocation() = default;
NodeLocation::NodeLocation(const char* pos) : _begin(pos),
											  _end(pos) {}
NodeLocation::NodeLocation(const char* begin, const char* end) : _begin(begin),
																 _end(end) {}

NodeLocation::NodeLocation(const NodeLocation&) noexcept = default;
NodeLocation& NodeLocation::operator=(const NodeLocation&) = default;

NodeLocation::NodeLocation(NodeLocation&&) = default;
NodeLocation& NodeLocation::operator=(NodeLocation&&) = default;

const char* NodeLocation::begin() const { return _begin; }
const char* NodeLocation::end() const { return _end; }

bool NodeLocation::is_synthesized() const { return _begin == nullptr && _end == nullptr; }

NodeLocation NodeLocation::make_from(const char* begin, const char* end) {
	end++;
	if (begin >= end) return NodeLocation(begin);
	return NodeLocation(begin, end);
}
