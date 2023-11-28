#pragma once

#include <concepts>
#include <cstdarg>
#include <cstdio>
#include <ostream>
#include <string>
#include <utility>

#include <openvic-dataloader/AbstractSyntaxTree.hpp>
#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/LexyReportError.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>

#include <lexy/input/base.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/visualize.hpp>

#include <dryad/_detail/config.hpp>
#include <dryad/abstract_node.hpp>
#include <dryad/arena.hpp>
#include <dryad/node.hpp>
#include <dryad/tree.hpp>

#include <fmt/core.h>

#include "openvic-dataloader/detail/CallbackOStream.hpp"
#include "openvic-dataloader/detail/utility/ErrorRange.hpp"
#include "openvic-dataloader/detail/utility/Utility.hpp"

#include <lexy_ext/report_error.hpp>

namespace ovdl {
	struct DiagnosticLogger {
		using AnnotationKind = lexy_ext::annotation_kind;
		using DiagnosticKind = lexy_ext::diagnostic_kind;

		using error_range = detail::error_range;

		explicit operator bool() const;
		bool errored() const;
		bool warned() const;

		NodeLocation location_of(const error::Error* error) const;

		template<std::derived_from<DiagnosticLogger> Logger>
		struct ErrorCallback {
			ErrorCallback(Logger& logger) : _logger(&logger) {}

			struct sink_t {
				using return_type = std::size_t;

				template<typename Input, typename Tag>
				void operator()(lexy::error_context<Input> const& context, lexy::error_for<Input, Tag> const& error) {
					using Reader = lexy::input_reader<Input>;
					error::Error* result;

					if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
						auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
						NodeLocation loc = NodeLocation::make_from(string.begin(), string.end());
						auto message = _logger.intern_cstr(fmt::format("expected '{}'", string.data()));
						result = _logger.template create<error::ExpectedLiteral>(loc, message, context.production());
					} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
						auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
						NodeLocation loc = NodeLocation::make_from(string.begin(), string.end());
						auto message = _logger.intern_cstr(fmt::format("expected keyword '{}'", string.data()));
						result = _logger.template create<error::ExpectedKeyword>(loc, message, context.production());
					} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
						auto message = _logger.intern_cstr(fmt::format("expected {}", error.name()));
						result = _logger.template create<error::ExpectedCharClass>(error.position(), message, context.production());
					} else {
						NodeLocation loc = NodeLocation::make_from(error.begin(), error.end());
						auto message = _logger.intern_cstr(error.message());
						result = _logger.template create<error::GenericParseError>(loc, message, context.production());
					}

					if constexpr (requires { _logger.insert(result); }) {
						_logger.insert(result);
					}

					_count++;
				}

				std::size_t finish() && {
					return _count;
				}

				Logger& _logger;
				std::size_t _count;
			};

			constexpr auto sink() const {
				return sink_t { *_logger, 0 };
			}

			mutable Logger* _logger;
		};

		template<typename T, typename... Args>
		T* create(NodeLocation loc, Args&&... args) {
			using node_creator = dryad::node_creator<decltype(DRYAD_DECLVAL(T).kind()), void>;
			T* result = _tree.create<T>(DRYAD_FWD(args)...);
			_map.insert(result, loc);
			return result;
		}

		template<typename T>
		T* create() {
			using node_creator = dryad::node_creator<decltype(DRYAD_DECLVAL(T).kind()), void>;
			T* result = _tree.create<T>();
			return result;
		}

	protected:
		bool _errored = false;
		bool _warned = false;
		dryad::node_map<const error::Error, NodeLocation> _map;
		dryad::tree<error::Root> _tree;

		struct SymbolId;
		using index_type = std::uint32_t;
		using symbol_type = dryad::symbol<SymbolId, index_type>;
		using symbol_interner_type = dryad::symbol_interner<SymbolId, char, index_type>;
		symbol_interner_type _symbol_interner;

	public:
		symbol_type intern(const char* str, std::size_t length) {
			return _symbol_interner.intern(str, length);
		}
		symbol_type intern(std::string_view str) {
			return intern(str.data(), str.size());
		}
		const char* intern_cstr(const char* str, std::size_t length) {
			return intern(str, length).c_str(_symbol_interner);
		}
		const char* intern_cstr(std::string_view str) {
			return intern_cstr(str.data(), str.size());
		}
		symbol_interner_type& symbol_interner() {
			return _symbol_interner;
		}
		const symbol_interner_type& symbol_interner() const {
			return _symbol_interner;
		}
	};

	template<IsFile FileT>
	struct BasicDiagnosticLogger : DiagnosticLogger {
		using file_type = FileT;

		template<typename... Args>
		using format_str = fmt::basic_format_string<char, fmt::type_identity_t<Args>...>;

		explicit BasicDiagnosticLogger(const file_type& file)
			: _file(&file) {
			_tree.set_root(_tree.create<error::Root>());
		}

		struct Writer;

		template<typename... Args>
		Writer error(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::error, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer warning(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::warning, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer note(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::note, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer info(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::info, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer debug(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::debug, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer fixit(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::fixit, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer help(format_str<Args...> fmt, Args&&... args) {
			return log(DiagnosticKind::help, fmt, std::forward<Args>(args)...);
		}

		Writer error(std::string_view sv) {
			return log(DiagnosticKind::error, fmt::runtime(sv));
		}

		Writer warning(std::string_view sv) {
			return log(DiagnosticKind::warning, fmt::runtime(sv));
		}

		Writer note(std::string_view sv) {
			return log(DiagnosticKind::note, fmt::runtime(sv));
		}

		Writer info(std::string_view sv) {
			return log(DiagnosticKind::info, fmt::runtime(sv));
		}

		Writer debug(std::string_view sv) {
			return log(DiagnosticKind::debug, fmt::runtime(sv));
		}

		Writer fixit(std::string_view sv) {
			return log(DiagnosticKind::fixit, fmt::runtime(sv));
		}

		Writer help(std::string_view sv) {
			return log(DiagnosticKind::help, fmt::runtime(sv));
		}

		auto error_callback() {
			return ErrorCallback(*this);
		}

		template<typename CharT>
		static void _write_to_buffer(const CharT* s, std::streamsize n, void* output_str) {
			auto* output = reinterpret_cast<std::basic_string<CharT>*>(output_str);
			output->append(s, n);
		}

		template<typename CharT>
		auto make_callback_stream(std::basic_string<CharT>& output) {
			return detail::make_callback_stream<CharT>(&_write_to_buffer<CharT>, reinterpret_cast<void*>(&output));
		}

		template<typename CharT>
		detail::OStreamOutputIterator make_ostream_iterator(std::basic_ostream<CharT>& stream) {
			return detail::OStreamOutputIterator { stream };
		}

		struct Writer {
			template<typename... Args>
			[[nodiscard]] Writer& primary(NodeLocation loc, format_str<Args...> fmt, Args&&... args) {
				return annotation(AnnotationKind::primary, loc, fmt, std::forward<Args>(args)...);
			}

			template<typename... Args>
			[[nodiscard]] Writer& secondary(NodeLocation loc, format_str<Args...> fmt, Args&&... args) {
				return annotation(AnnotationKind::secondary, loc, fmt, std::forward<Args>(args)...);
			}

			[[nodiscard]] Writer& primary(NodeLocation loc, std::string_view sv) {
				return annotation(AnnotationKind::primary, loc, fmt::runtime(sv));
			}

			[[nodiscard]] Writer& secondary(NodeLocation loc, std::string_view sv) {
				return annotation(AnnotationKind::secondary, loc, fmt::runtime(sv));
			}

			void finish() {}

			template<typename... Args>
			[[nodiscard]] Writer& annotation(AnnotationKind kind, NodeLocation loc, format_str<Args...> fmt, Args&&... args) {
				auto begin_loc = lexy::get_input_location(_file->buffer(), loc.begin());

				std::basic_string<typename decltype(fmt.get())::value_type> output;
				auto stream = _logger.make_callback_stream(output);
				auto iter = _logger.make_ostream_iterator(stream);

				_impl.write_empty_annotation(iter);
				_impl.write_annotation(iter, kind, begin_loc, loc.end(),
					[&](auto out, lexy::visualization_options) {
						return lexy::_detail::write_str(out, fmt::format(fmt, std::forward<Args>(args)...).c_str());
					});

				error::Annotation* annotation;
				auto message = _logger.intern_cstr(output);
				switch (kind) {
					case AnnotationKind::primary:
						_logger.create<error::PrimaryAnnotation>(loc, message);
						break;
					case AnnotationKind::secondary:
						_logger.create<error::SecondaryAnnotation>(loc, message);
						break;
					default: detail::unreachable();
				}
				_semantic->push_back(annotation);
				return *this;
			}

		private:
			Writer(BasicDiagnosticLogger& logger, const file_type* file, error::Semantic* semantic)
				: _file(file),
				  _impl(file->buffer(), { lexy::visualize_fancy }),
				  _logger(logger),
				  _semantic(semantic) {}

			const file_type* _file;
			lexy_ext::diagnostic_writer<lexy::buffer<typename file_type::encoding_type>> _impl;
			BasicDiagnosticLogger& _logger;
			error::Semantic* _semantic;

			friend BasicDiagnosticLogger;
		};

		using diagnostic_writer = lexy_ext::diagnostic_writer<lexy::buffer<typename file_type::encoding_type>>;

		template<std::derived_from<error::Error> T, typename... Args>
		void log_with_impl(diagnostic_writer& impl, T* error, DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			std::basic_string<typename decltype(fmt.get())::value_type> output;
			auto stream = make_callback_stream(output);
			auto iter = make_ostream_iterator(stream);

			impl.write_message(iter, kind,
				[&](auto out, lexy::visualization_options) {
					return lexy::_detail::write_str(out, fmt::format(fmt, std::forward<Args>(args)...).c_str());
				});
			impl.write_path(iter, _file->path());

			auto message = intern_cstr(output);
			error->_set_message(message);
			insert(error);
		}

		template<std::derived_from<error::Error> T, typename... Args>
		void log_with_error(T* error, DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			auto impl = diagnostic_writer { _file->buffer() };
			log_with_impl(impl, error, kind, fmt, std::forward<Args>(args)...);
		}

		template<std::derived_from<error::Error> T, typename... Args>
		void create_log(DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			log_with_error(create<T>(), kind, fmt, std::forward<Args>(args)...);
		}

		template<typename... Args>
		Writer log(DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			error::Semantic* semantic;

			switch (kind) {
				case DiagnosticKind::error:
					semantic = create<error::SemanticError>();
					break;
				case DiagnosticKind::warning:
					semantic = create<error::SemanticWarning>();
					break;
				case DiagnosticKind::info:
					semantic = create<error::SemanticInfo>();
					break;
				case DiagnosticKind::debug:
					semantic = create<error::SemanticDebug>();
					break;
				case DiagnosticKind::fixit:
					semantic = create<error::SemanticFixit>();
					break;
				case DiagnosticKind::help:
					semantic = create<error::SemanticHelp>();
					break;
				default: detail::unreachable();
			}

			Writer result(*this, _file, semantic);

			log_with_impl(result._impl, semantic, kind, fmt, std::forward<Args>(args)...);

			if (kind == DiagnosticKind::error)
				_errored = true;
			if (kind == DiagnosticKind::warning)
				_warned = true;

			return result;
		}

		error_range get_errors() const {
			return _tree.root()->errors();
		}

	private:
		void insert(error::Error* root) {
			_tree.root()->insert_back(root);
		}

		const file_type* _file;
	};
}