#pragma once

#include <concepts>
#include <cstdint>
#include <initializer_list>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>
#include <vector>

#include <fmt/compile.h>
#include <fmt/core.h>
#include <fmt/format.h>

namespace ovdl::csv {
	template<typename T, typename Value>
	concept IsMap =
		requires(T t) {
			{ t.begin() } -> std::same_as<typename T::const_iterator>;
			{ t.end() } -> std::same_as<typename T::const_iterator>;
			{ t.empty() } -> std::same_as<bool>;
			{ t.size() } -> std::same_as<typename T::size_type>;
			{ t.at("") } -> std::same_as<const Value&>;
			{ t.find("") } -> std::same_as<typename T::const_iterator>;
			{ t[""] } -> std::same_as<const Value&>;
		};

	class ValueNode {
	public:
		struct Placeholder {
			static constexpr char ESCAPE_CHAR = '$';
			static constexpr std::string_view ESCAPE_STR = std::string_view { &ESCAPE_CHAR, 1 };

			std::string value;
			inline std::string as_string(std::string_view prefix = ESCAPE_STR, std::optional<std::string_view> suffix = std::nullopt) const {
				return fmt::format(FMT_COMPILE("{}{}{}"), prefix, value, suffix.value_or(prefix));
			}
		};

		using internal_value_type = std::variant<std::string, Placeholder>;
		using position_type = std::uint32_t;

		ValueNode();
		ValueNode(std::string_view string, position_type position = 0);
		ValueNode(std::vector<internal_value_type> value_list, position_type position = 0);
		ValueNode(std::initializer_list<internal_value_type> value_list, position_type position = 0);

		void set_position(position_type position);
		position_type get_position() const;

		void set_as_list(internal_value_type value);
		void add_to_list(internal_value_type value);
		bool list_is_empty() const;

		inline std::string make_from_map(const IsMap<std::string> auto& map) const {
			std::vector<std::string_view> pre_joined(_value_list.size());

			for (auto&& value : _value_list) {
				pre_joined.push_back(std::visit([&](auto&& arg) -> std::string_view {
					using T = std::decay_t<decltype(arg)>;
					if constexpr (std::is_same_v<T, std::string>) {
						return arg;
					} else if constexpr (std::is_same_v<T, Placeholder>) {
						return map[arg.value];
					}
				},
					value));
			}

			return fmt::format(FMT_COMPILE("{}"), fmt::join(pre_joined, ""));
		}

		std::string make(std::string_view prefix = Placeholder::ESCAPE_STR, std::optional<std::string_view> suffix = std::nullopt) const;

	private:
		position_type _position;
		std::vector<internal_value_type> _value_list;
	};
}