#pragma once

#include <utility>

#include <openvic-dataloader/detail/Encoding.hpp>

#include <lexy/encoding.hpp>
#include <lexy/input/buffer.hpp>

#include <dryad/tree.hpp>

#include "DiagnosticLogger.hpp"
#include "detail/InternalConcepts.hpp"

namespace ovdl {
	struct BasicParseState {
		explicit BasicParseState(detail::Encoding encoding = detail::Encoding::Unknown) : _encoding(encoding) {}

		detail::Encoding encoding() const {
			return _encoding;
		}

	protected:
		detail::Encoding _encoding;
	};

	template<detail::IsAst AstT>
	struct ParseState : BasicParseState {
		using ast_type = AstT;
		using file_type = typename ast_type::file_type;
		using diagnostic_logger_type = BasicDiagnosticLogger<ParseState>;

		ParseState() : _ast {}, _logger { this->ast().file() } {}

		ParseState(typename ast_type::file_type&& file, detail::Encoding encoding)
			: _ast { std::move(file) },
			  _logger { this->ast().file() },
			  BasicParseState(encoding) {}

		template<typename Encoding, typename MemoryResource = void>
		ParseState(lexy::buffer<Encoding, MemoryResource>&& buffer, detail::Encoding encoding)
			: ParseState(typename ast_type::file_type { std::move(buffer) }, encoding) {}

		template<typename Encoding, typename MemoryResource = void>
		ParseState(const char* path, lexy::buffer<Encoding, MemoryResource>&& buffer, detail::Encoding encoding)
			: ParseState(typename ast_type::file_type { path, std::move(buffer) }, encoding) {}

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

		void update_logger_file() {
			_logger.set_file(this->ast().file());
		}

	private:
		ast_type _ast;
		diagnostic_logger_type _logger;
	};

	template<detail::IsFile FileT>
	struct FileParseState : BasicParseState {
		using file_type = FileT;
		using diagnostic_logger_type = BasicDiagnosticLogger<FileParseState>;

		FileParseState() : _file {}, _logger { this->file() } {}

		FileParseState(file_type&& file, detail::Encoding encoding)
			: _file { std::move(file) },
			  _logger { this->file() },
			  BasicParseState(encoding) {}

		template<typename Encoding, typename MemoryResource = void>
		FileParseState(lexy::buffer<Encoding, MemoryResource>&& buffer, detail::Encoding encoding)
			: FileParseState(file_type { std::move(buffer) }, encoding) {}

		template<typename Encoding, typename MemoryResource = void>
		FileParseState(const char* path, lexy::buffer<Encoding, MemoryResource>&& buffer, detail::Encoding encoding)
			: FileParseState(file_type { path, std::move(buffer) }, encoding) {}

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

		void update_logger_file() {
			_logger.set_file(this->file());
		}

	private:
		file_type _file;
		diagnostic_logger_type _logger;
	};
}