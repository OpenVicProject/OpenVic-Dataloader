#pragma once

#include <cstdint>
#include <tuple>
#include <type_traits>
#include <variant>

#include <openvic-dataloader/detail/Concepts.hpp>

#if __has_cpp_attribute(msvc::no_unique_address)
#define OVDL_NO_UNIQUE_ADDRESS                                  \
	_Pragma("warning(push)") _Pragma("warning(disable : 4848)") \
		[[msvc::no_unique_address]] _Pragma("warning(pop)")
#else
#define OVDL_NO_UNIQUE_ADDRESS [[no_unique_address]]
#endif

namespace ovdl::detail {
	[[noreturn]] inline void unreachable() {
		// Uses compiler specific extensions if possible.
		// Even if no extension is used, undefined behavior is still raised by
		// an empty function body and the noreturn attribute.
#ifdef __GNUC__ // GCC, Clang, ICC
		__builtin_unreachable();
#elif defined(_MSC_VER) // MSVC
		__assume(false);
#endif
	}

	template<typename EnumT>
		requires std::is_enum_v<EnumT>
	constexpr std::underlying_type_t<EnumT> to_underlying(EnumT e) {
		return static_cast<std::underlying_type_t<EnumT>>(e);
	}

	template<typename EnumT>
		requires std::is_enum_v<EnumT>
	constexpr EnumT from_underlying(std::underlying_type_t<EnumT> ut) {
		return static_cast<EnumT>(ut);
	}

	template<typename Type, typename... Types>
	struct TypeRegister {
		using tuple_type = std::tuple<Type, Types...>;
		using variant_type = std::variant<Type, Types...>;

		template<typename QueriedType>
		struct _id_getter {
			static constexpr std::uint32_t type_id() {
				static_assert(any_of<QueriedType, Type, Types...>, "Cannot query an non-registered type");

				if constexpr (std::is_same_v<Type, QueriedType>) {
					return 0;
				} else {
					return 1 + TypeRegister<Types...>::template _id_getter<QueriedType>::type_id();
				}
			};
		};

		template<typename QueriedType>
		static constexpr std::uint32_t type_id() {

			return _id_getter<QueriedType>::type_id();
		}

		template<std::uint32_t Id>
		using type_by_id = std::tuple_element_t<Id, tuple_type>;
	};

	template<typename...>
	struct type_concat;

	template<typename... Ts, template<typename...> typename TT, typename... TTs>
	struct type_concat<TT<TTs...>, Ts...> {
		using type = TT<TTs..., Ts...>;
	};

	template<typename... Ts>
	using type_concat_t = typename type_concat<Ts...>::type;

	template<typename...>
	struct type_prepend;

	template<typename... Ts, template<typename...> typename TT, typename... TTs>
	struct type_prepend<TT<TTs...>, Ts...> {
		using type = TT<Ts..., TTs...>;
	};

	template<typename... Ts>
	using type_prepend_t = typename type_prepend<Ts...>::type;

	template<typename Type, template<typename...> typename Template>
	struct is_instance_of : std::false_type {};

	template<typename... Ts, template<typename...> typename Template>
	struct is_instance_of<Template<Ts...>, Template> : std::true_type {};

	template<typename Type, template<typename...> typename Template>
	static constexpr auto is_instance_of_v = is_instance_of<Type, Template>::value;

	template<typename T, template<typename...> typename Template>
	concept InstanceOf = is_instance_of_v<std::remove_cv_t<std::remove_reference_t<T>>, Template>;
}