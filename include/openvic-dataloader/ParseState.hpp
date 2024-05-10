#pragma once

#include <concepts>

#include <openvic-dataloader/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/DiagnosticLogger.hpp>

#include <dryad/tree.hpp>

namespace ovdl {
	template<typename T>
	concept IsParseState = requires(
		T t,
		const T ct,
		typename T::ast_type::file_type&& file,
		lexy::buffer<typename T::ast_type::file_type::encoding_type>&& buffer,
		const char* path //
	) {
		requires IsAst<typename T::ast_type>;
		requires std::derived_from<typename T::diagnostic_logger_type, DiagnosticLogger>;
		{ T { std::move(file) } } -> std::same_as<T>;
		{ T { std::move(buffer) } } -> std::same_as<T>;
		{ T { path, std::move(buffer) } } -> std::same_as<T>;
		{ t.ast() } -> std::same_as<typename T::ast_type&>;
		{ ct.ast() } -> std::same_as<const typename T::ast_type&>;
		{ t.logger() } -> std::same_as<typename T::diagnostic_logger_type&>;
		{ ct.logger() } -> std::same_as<const typename T::diagnostic_logger_type&>;
	};

	template<IsAst AstT>
	struct ParseState {
		using ast_type = AstT;
		using diagnostic_logger_type = BasicDiagnosticLogger<typename ast_type::file_type>;

		ParseState(typename ast_type::file_type&& file)
			: _ast { std::move(file) },
			  _logger { _ast.file() } {}

		ParseState(lexy::buffer<typename ast_type::file_type::encoding_type>&& buffer)
			: ParseState(typename ast_type::file_type { std::move(buffer) }) {}

		ParseState(const char* path, lexy::buffer<typename ast_type::file_type::encoding_type>&& buffer)
			: ParseState(typename ast_type::file_type { path, std::move(buffer) }) {}

		ast_type& ast() {
			return _ast;
		}

		const ast_type& ast() const {
			return _ast;
		}

		diagnostic_logger_type& logger() {
			return _logger;
		}

		const diagnostic_logger_type& logger() const {
			return _logger;
		}

	private:
		ast_type _ast;
		diagnostic_logger_type _logger;
	};

	template<typename T>
	concept IsFileParseState = requires(
		T t,
		const T ct,
		typename T::file_type&& file,
		lexy::buffer<typename T::file_type::encoding_type>&& buffer,
		const char* path //
	) {
		requires IsFile<typename T::file_type>;
		requires std::derived_from<typename T::diagnostic_logger_type, DiagnosticLogger>;
		{ T { std::move(file) } } -> std::same_as<T>;
		{ T { std::move(buffer) } } -> std::same_as<T>;
		{ T { path, std::move(buffer) } } -> std::same_as<T>;
		{ t.file() } -> std::same_as<typename T::file_type&>;
		{ ct.file() } -> std::same_as<const typename T::file_type&>;
		{ t.logger() } -> std::same_as<typename T::diagnostic_logger_type&>;
		{ ct.logger() } -> std::same_as<const typename T::diagnostic_logger_type&>;
	};

	template<IsFile FileT>
	struct FileParseState {
		using file_type = FileT;
		using diagnostic_logger_type = BasicDiagnosticLogger<file_type>;

		FileParseState(file_type&& file)
			: _file { std::move(file) },
			  _logger { file } {}

		FileParseState(lexy::buffer<typename file_type::encoding_type>&& buffer)
			: FileParseState(file_type { std::move(buffer) }) {}

		FileParseState(const char* path, lexy::buffer<typename file_type::encoding_type>&& buffer)
			: FileParseState(file_type { path, std::move(buffer) }) {}

		file_type& file() {
			return _file;
		}

		const file_type& file() const {
			return _file;
		}

		diagnostic_logger_type& logger() {
			return _logger;
		}

		const diagnostic_logger_type& logger() const {
			return _logger;
		}

	private:
		file_type _file;
		diagnostic_logger_type _logger;
	};
}