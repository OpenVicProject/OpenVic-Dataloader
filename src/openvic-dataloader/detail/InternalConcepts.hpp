#pragma once

#include <concepts>
#include <utility>

#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/Encoding.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>

#include <dryad/symbol.hpp>

#include <fmt/core.h>

#include <lexy_ext/report_error.hpp>

namespace ovdl::detail {
	template<typename T>
	concept IsFile =
		requires(T t, const T ct, const typename T::node_type* node, NodeLocation location) {
			typename T::node_type;
			{ ct.size() } -> std::same_as<size_t>;
			{ t.set_location(node, location) } -> std::same_as<void>;
			{ t.location_of(node) } -> std::same_as<NodeLocation>;
		};

	template<typename T>
	concept IsAst =
		requires(
			T t,
			const T ct,
			const typename T::node_type* node,
			NodeLocation loc //
		) {
			requires IsFile<typename T::file_type>;
			typename T::root_node_type;
			typename T::node_type;
			requires std::derived_from<typename T::root_node_type, typename T::node_type>;
			{ t.set_location(node, loc) } -> std::same_as<void>;
			{ t.location_of(node) } -> std::same_as<NodeLocation>;
			{ t.root() } -> std::same_as<typename T::root_node_type*>;
			{ ct.root() } -> std::same_as<const typename T::root_node_type*>;
			{ t.file() } -> std::same_as<typename T::file_type&>;
			{ ct.file() } -> std::same_as<const typename T::file_type&>;
		};

	template<typename T>
	concept IsDiagnosticLogger = requires(
		T t,
		const T ct,
		const char* str,
		std::size_t length,
		std::string_view sv,
		lexy_ext::diagnostic_kind diag_kind //
	) {
		typename T::error_range;
		typename T::Writer;
		{ static_cast<bool>(ct) } -> std::same_as<bool>;
		{ ct.errored() } -> std::same_as<bool>;
		{ ct.warned() } -> std::same_as<bool>;
		{ ct.get_errors() } -> std::same_as<typename T::error_range>;
		{ t.intern(str, length) } -> detail::InstanceOf<dryad::symbol>;
		{ t.intern(sv) } -> detail::InstanceOf<dryad::symbol>;
		{ t.intern_cstr(str, length) } -> std::same_as<const char*>;
		{ t.intern_cstr(sv) } -> std::same_as<const char*>;
		{ t.symbol_interner() } -> detail::InstanceOf<dryad::symbol_interner>;
		{ ct.symbol_interner() } -> detail::InstanceOf<dryad::symbol_interner>;
		{ t.error(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.warning(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.note(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.info(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.debug(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.fixit(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.help(std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
		{ t.error(sv) } -> std::same_as<typename T::Writer>;
		{ t.warning(sv) } -> std::same_as<typename T::Writer>;
		{ t.note(sv) } -> std::same_as<typename T::Writer>;
		{ t.info(sv) } -> std::same_as<typename T::Writer>;
		{ t.debug(sv) } -> std::same_as<typename T::Writer>;
		{ t.fixit(sv) } -> std::same_as<typename T::Writer>;
		{ t.help(sv) } -> std::same_as<typename T::Writer>;
		{ std::move(t.error_callback().sink()).finish() } -> std::same_as<std::size_t>;
		{ t.log(diag_kind, std::declval<typename T::template format_str<>>()) } -> std::same_as<typename T::Writer>;
	};

	template<typename T>
	concept IsParseState = requires(
		T t,
		const T ct,
		typename T::ast_type::file_type&& file,
		lexy::buffer<lexy::default_encoding>&& buffer,
		ovdl::detail::Encoding encoding,
		const char* path //
	) {
		requires IsAst<typename T::ast_type>;
		requires IsDiagnosticLogger<typename T::diagnostic_logger_type>;
		{ T { std::move(file), encoding } } -> std::same_as<T>;
		{ T { std::move(buffer), encoding } } -> std::same_as<T>;
		{ T { path, std::move(buffer), encoding } } -> std::same_as<T>;
		{ t.ast() } -> std::same_as<typename T::ast_type&>;
		{ ct.ast() } -> std::same_as<const typename T::ast_type&>;
		{ t.logger() } -> std::same_as<typename T::diagnostic_logger_type&>;
		{ ct.logger() } -> std::same_as<const typename T::diagnostic_logger_type&>;
	};

	template<typename T>
	concept IsFileParseState = requires(
		T t,
		const T ct,
		typename T::file_type&& file,
		lexy::buffer<lexy::default_encoding>&& buffer,
		ovdl::detail::Encoding encoding,
		const char* path //
	) {
		requires IsFile<typename T::file_type>;
		requires IsDiagnosticLogger<typename T::diagnostic_logger_type>;
		{ T { std::move(file), encoding } } -> std::same_as<T>;
		{ T { std::move(buffer), encoding } } -> std::same_as<T>;
		{ T { path, std::move(buffer), encoding } } -> std::same_as<T>;
		{ t.file() } -> std::same_as<typename T::file_type&>;
		{ ct.file() } -> std::same_as<const typename T::file_type&>;
		{ t.logger() } -> std::same_as<typename T::diagnostic_logger_type&>;
		{ ct.logger() } -> std::same_as<const typename T::diagnostic_logger_type&>;
	};

	template<typename T>
	concept IsStateType = IsParseState<T> || IsFileParseState<T>;
}