#pragma once

#include <concepts> // IWYU pragma: keep
#include <cstdio>
#include <iostream>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>

#include <openvic-dataloader/Error.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/CallbackOStream.hpp>
#include <openvic-dataloader/detail/ErrorRange.hpp>
#include <openvic-dataloader/detail/OStreamOutputIterator.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/Utility.hpp>

#include <lexy/error.hpp>
#include <lexy/input/base.hpp>
#include <lexy/input/buffer.hpp>
#include <lexy/input_location.hpp>
#include <lexy/lexeme.hpp>
#include <lexy/visualize.hpp>

#include <dryad/node_map.hpp>
#include <dryad/tree.hpp>

#include <fmt/core.h>

#include <lexy_ext/report_error.hpp>

namespace ovdl {
	template<typename ParseState>
	struct BasicDiagnosticLogger;

	struct DiagnosticLogger : error::ErrorSymbolInterner {
		using AnnotationKind = lexy_ext::annotation_kind;
		using DiagnosticKind = lexy_ext::diagnostic_kind;

		using error_range = detail::error_range<error::Root>;

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
					using Encoding = typename Reader::encoding;
					using char_type = typename Encoding::char_type;
					error::Error* result;

					std::string production_name = context.production();
					auto left_strip = production_name.find_first_of('<');
					if (left_strip != std::string::npos) {
						auto right_strip = production_name.find_first_of('>', left_strip);
						if (right_strip != std::string::npos) {
							production_name.erase(left_strip, right_strip - left_strip + 1);
						}
					}

					auto context_location = lexy::get_input_location(context.input(), context.position());
					auto location = lexy::get_input_location(context.input(), error.position(), context_location.anchor());

					if constexpr (detail::is_instance_of_v<Logger, BasicDiagnosticLogger>) {
						lexy_ext::diagnostic_writer impl { context.input() };

						BasicNodeLocation loc = [&] {
							if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
								return BasicNodeLocation<char_type>::make_from(error.position(), error.position() + error.index() + 1);
							} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
								return BasicNodeLocation<char_type>::make_from(error.position(), error.end());
							} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
								return BasicNodeLocation<char_type>::make_from(error.position(), error.position() + 1);
							} else {
								return BasicNodeLocation<char_type>::make_from(error.position(), error.end());
							}
						}();

						auto writer = _logger.template parse_error<Tag>(impl, loc, production_name.c_str());
						if (location.line_nr() != context_location.line_nr()) {
							writer.secondary(BasicNodeLocation { context.position(), lexy::_detail::next(context.position()) }, "beginning here").finish();
						}

						if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
							auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
							writer.primary(loc, "expected '{}'", string.data())
								.finish();
						} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
							auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
							writer.primary(loc, "expected keyword '{}'", string.data())
								.finish();
						} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
							writer.primary(loc, "expected {}", error.name())
								.finish();
						} else {
							writer.primary(loc, error.message())
								.finish();
						}
						result = writer.error();
					} else {
						auto production = production_name;
						if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
							auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
							NodeLocation loc = NodeLocation::make_from(context.position(), error.position() - 1);
							auto message = _logger.intern(fmt::format("expected '{}'", string.data()));
							result = _logger.template create<error::ExpectedLiteral>(loc, message, production);
						} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
							auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());
							NodeLocation loc = NodeLocation::make_from(context.position(), error.position() - 1);
							auto message = _logger.intern(fmt::format("expected keyword '{}'", string.data()));
							result = _logger.template create<error::ExpectedKeyword>(loc, message, production);
						} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
							auto message = _logger.intern(fmt::format("expected {}", error.name()));
							result = _logger.template create<error::ExpectedCharClass>(error.position(), message, production);
						} else {
							NodeLocation loc = NodeLocation::make_from(error.begin(), error.end());
							auto message = _logger.intern(error.message());
							result = _logger.template create<error::GenericParseError>(loc, message, production);
						}
					}
					_logger.insert(result);

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

		template<typename T, typename LocCharT, typename... Args>
		T* create(BasicNodeLocation<LocCharT> loc, Args&&... args) {
			using node_creator = dryad::node_creator<decltype(std::declval<T>().kind()), void>;
			T* result = _tree.create<T>(static_cast<decltype(args)>(args)...);
			_map.insert(result, loc);
			return result;
		}

		template<typename T>
		T* create() {
			using node_creator = dryad::node_creator<decltype(std::declval<T>().kind()), void>;
			T* result = _tree.create<T>();
			return result;
		}

		error_range get_errors() const {
			return _tree.root()->errors();
		}

	protected:
		bool _errored = false;
		bool _warned = false;
		dryad::node_map<const error::Error, NodeLocation> _map;
		dryad::tree<error::Root> _tree;

		symbol_interner_type _symbol_interner;

		void insert(error::Error* root) {
			_tree.root()->insert_back(root);
		}

	public:
		symbol_type intern(const char* str, std::size_t length) {
			return _symbol_interner.intern(str, length);
		}
		symbol_type intern(std::string_view str) {
			return intern(str.data(), str.size());
		}
		const char* intern_cstr(const char* str, std::size_t length) {
			return intern(str, length).c_str();
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

		template<typename Reader>
		symbol_type intern(lexy::lexeme<Reader> lexeme) {
			return intern(lexeme.data(), lexeme.size());
		}
		template<typename Reader>
		const char* intern_cstr(lexy::lexeme<Reader> lexeme) {
			return intern_cstr(lexeme.data(), lexeme.size());
		}
	};

	template<typename ParseState>
	struct BasicDiagnosticLogger : DiagnosticLogger {
		using parse_state_type = ParseState;
		using file_type = typename parse_state_type::file_type;

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
			template<typename LocCharT, typename... Args>
			[[nodiscard]] Writer& primary(BasicNodeLocation<LocCharT> loc, format_str<Args...> fmt, Args&&... args) {
				return annotation(AnnotationKind::primary, loc, fmt, std::forward<Args>(args)...);
			}

			template<typename LocCharT, typename... Args>
			[[nodiscard]] Writer& secondary(BasicNodeLocation<LocCharT> loc, format_str<Args...> fmt, Args&&... args) {
				return annotation(AnnotationKind::secondary, loc, fmt, std::forward<Args>(args)...);
			}

			template<typename LocCharT>
			[[nodiscard]] Writer& primary(BasicNodeLocation<LocCharT> loc, const char* sv) {
				return annotation(AnnotationKind::primary, loc, fmt::runtime(sv));
			}

			template<typename LocCharT>
			[[nodiscard]] Writer& secondary(BasicNodeLocation<LocCharT> loc, const char* sv) {
				return annotation(AnnotationKind::secondary, loc, fmt::runtime(sv));
			}

			void finish() {}

			template<typename LocCharT, typename... Args>
			[[nodiscard]] Writer& annotation(AnnotationKind kind, BasicNodeLocation<LocCharT> loc, format_str<Args...> fmt, Args&&... args) {
				std::basic_string<typename decltype(fmt.get())::value_type> output;

				_file.visit_buffer([&](auto&& buffer) {
					using char_type = typename std::decay_t<decltype(buffer)>::encoding::char_type;

					BasicNodeLocation<char_type> converted_loc = loc;

					auto begin_loc = lexy::get_input_location(buffer, converted_loc.begin());

					auto stream = _logger.make_callback_stream(output);
					auto iter = _logger.make_ostream_iterator(stream);

					lexy_ext::diagnostic_writer _impl { buffer, { lexy::visualize_fancy } };
					_impl.write_empty_annotation(iter);
					_impl.write_annotation(iter, kind, begin_loc, converted_loc.end(),
						[&](auto out, lexy::visualization_options) {
							return lexy::_detail::write_str(out, fmt::format(fmt, std::forward<Args>(args)...).c_str());
						});
				});

				error::Annotation* annotation;
				auto message = _logger.intern(output);
				switch (kind) {
					case AnnotationKind::primary:
						annotation = _logger.create<error::PrimaryAnnotation>(loc, message);
						break;
					case AnnotationKind::secondary:
						annotation = _logger.create<error::SecondaryAnnotation>(loc, message);
						break;
					default: detail::unreachable();
				}
				_annotated->push_back(annotation);
				return *this;
			}

			error::AnnotatedError* error() {
				return _annotated;
			}

		private:
			Writer(BasicDiagnosticLogger& logger, const file_type& file, error::AnnotatedError* annotated)
				: _file(file),
				  _logger(logger),
				  _annotated(annotated) {}

			const file_type& _file;
			BasicDiagnosticLogger& _logger;
			error::AnnotatedError* _annotated;

			friend BasicDiagnosticLogger;
		};

		template<std::derived_from<error::Error> T, typename Buffer, typename... Args>
		void log_with_impl(lexy_ext::diagnostic_writer<Buffer>& impl, T* error, DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			std::basic_string<typename decltype(fmt.get())::value_type> output;
			auto stream = make_callback_stream(output);
			auto iter = make_ostream_iterator(stream);

			impl.write_message(iter, kind,
				[&](auto out, lexy::visualization_options) {
					return lexy::_detail::write_str(out, fmt::format(fmt, std::forward<Args>(args)...).c_str());
				});
			if constexpr (!std::same_as<T, error::BufferError>) {
				if (file().path() != nullptr && file().path()[0] != '\0') {
					impl.write_path(iter, file().path());
				}
			}

			output.pop_back();
			auto message = intern(output);
			error->_set_message(message);
			if (!error->is_linked_in_tree()) {
				insert(error);
			}
		}

		template<typename Tag, typename Buffer>
		Writer parse_error(lexy_ext::diagnostic_writer<Buffer>& impl, NodeLocation loc, const char* production_name) {
			std::basic_string<typename Buffer::encoding::char_type> output;
			auto stream = make_callback_stream(output);
			auto iter = make_ostream_iterator(stream);

			impl.write_message(iter, DiagnosticKind::error,
				[&](auto out, lexy::visualization_options) {
					return lexy::_detail::write_str(out, fmt::format("while parsing {}", production_name).c_str());
				});
			impl.write_path(iter, file().path());

			auto production = production_name;
			auto message = intern(output);
			auto* error = [&] {
				if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
					return create<error::ExpectedLiteral>(loc, message, production);
				} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
					return create<error::ExpectedKeyword>(loc, message, production);
				} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
					return create<error::ExpectedCharClass>(loc, message, production);
				} else {
					return create<error::GenericParseError>(loc, message, production);
				}
			}();

			Writer result(*this, file(), error);
			_errored = true;

			return result;
		}

		template<std::derived_from<error::Error> T, typename... Args>
		void log_with_error(T* error, DiagnosticKind kind, format_str<Args...> fmt, Args&&... args) {
			file().visit_buffer(
				[&](auto&& buffer) {
					lexy_ext::diagnostic_writer impl { buffer };
					log_with_impl(impl, error, kind, fmt, std::forward<Args>(args)...);
				});
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

			Writer result(*this, file(), semantic);

			file().visit_buffer([&](auto&& buffer) {
				lexy_ext::diagnostic_writer impl { buffer };
				log_with_impl(impl, semantic, kind, fmt, std::forward<Args>(args)...);
			});

			if (kind == DiagnosticKind::error) {
				_errored = true;
			}
			if (kind == DiagnosticKind::warning) {
				_warned = true;
			}

			return result;
		}

		const auto& file() const {
			return *_file;
		}

	private:
		const file_type* _file;
	};
}