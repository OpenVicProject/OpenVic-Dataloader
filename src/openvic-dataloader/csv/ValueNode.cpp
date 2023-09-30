#include <openvic-dataloader/csv/ValueNode.hpp>

using namespace ovdl;
using namespace ovdl::csv;

ValueNode::ValueNode() = default;

ValueNode::ValueNode(std::string_view string, position_type position)
	: ValueNode(std::initializer_list<internal_value_type> { std::string(string) }, position) {
}
ValueNode::ValueNode(std::vector<internal_value_type> value_list, position_type position)
	: _value_list(value_list),
	  _position(position) {
}
ValueNode::ValueNode(std::initializer_list<internal_value_type> value_list, position_type position)
	: _value_list(value_list),
	  _position(position) {
}

void ValueNode::set_position(position_type position) {
	_position = position;
}

ValueNode::position_type ValueNode::get_position() const {
	return _position;
}

void ValueNode::set_as_list(internal_value_type value) {
	_value_list.assign({ value });
}

void ValueNode::add_to_list(internal_value_type value) {
	_value_list.push_back(value);
}

bool ValueNode::list_is_empty() const {
	return _value_list.empty();
}

std::string ValueNode::make(std::string_view prefix, std::optional<std::string_view> suffix) const {
	std::vector<std::string> pre_joined(_value_list.size());

	for (auto&& value : _value_list) {
		const auto& result = std::visit([&](auto&& arg) -> std::string {
			using T = std::decay_t<decltype(arg)>;
			if constexpr (std::is_same_v<T, std::string>) {
				return arg;
			} else if constexpr (std::is_same_v<T, Placeholder>) {
				return arg.as_string(prefix, suffix);
			}
		},
			value);
		pre_joined.push_back(result);
	}

	return fmt::format("{}", fmt::join(pre_joined, ""));
}