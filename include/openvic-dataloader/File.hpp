#pragma once

#include <concepts>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/LexyFwdDeclaration.hpp>

#include <dryad/node_map.hpp>

namespace ovdl {
	template<typename T>
	concept IsEncoding = requires(T t) {
		typename T::char_type;
		typename T::int_type;
		{ T::template is_secondary_char_type<typename T::char_type>() } -> std::same_as<bool>;
		{ T::eof() } -> std::same_as<typename T::int_type>;
		{ T::to_int_type(typename T::char_type {}) } -> std::same_as<typename T::int_type>;
	};

	struct File {
		explicit File(const char* path);

		const char* path() const noexcept;

	protected:
		const char* _path;
	};

	template<typename T>
	concept IsFile =
		std::derived_from<T, File> && IsEncoding<typename T::encoding_type> &&
		requires(T t, const typename T::node_type* node, NodeLocation location) {
			{ t.buffer() } -> std::same_as<const lexy::buffer<typename T::encoding_type, void>&>;
			{ t.set_location(node, location) } -> std::same_as<void>;
			{ t.location_of(node) } -> std::same_as<NodeLocation>;
		};

	template<typename EncodingT, typename NodeT>
	struct BasicFile : File {
		using encoding_type = EncodingT;
		using node_type = NodeT;

		explicit BasicFile(const char* path, lexy::buffer<encoding_type, void>&& buffer)
			: File(path),
			  _buffer(static_cast<std ::remove_reference_t<decltype(buffer)>&&>(buffer)) {}

		explicit BasicFile(lexy::buffer<encoding_type, void>&& buffer)
			: File(""),
			  _buffer(static_cast<std ::remove_reference_t<decltype(buffer)>&&>(buffer)) {}

		const lexy::buffer<encoding_type, void>& buffer() const {
			return _buffer;
		}

		void set_location(const node_type* n, NodeLocation loc) {
			_map.insert(n, loc);
		}

		NodeLocation location_of(const node_type* n) const {
			auto result = _map.lookup(n);
			DRYAD_ASSERT(result != nullptr, "every Node should have a NodeLocation");
			return *result;
		}

	protected:
		lexy::buffer<encoding_type, void> _buffer;
		dryad::node_map<const node_type, NodeLocation> _map;
	};
}