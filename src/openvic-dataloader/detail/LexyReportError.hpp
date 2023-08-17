#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include "openvic-dataloader/ParseData.hpp"
#include <lexy/input_location.hpp>
#include <lexy/visualize.hpp>
#include <lexy_ext/report_error.hpp>
#include <openvic-dataloader/ParseError.hpp>

namespace ovdl::detail {
	template<typename OutputIterator>
	struct _ReportError {
		OutputIterator _iter;
		lexy::visualization_options _opts;
		const char* _path;

		struct _sink {
			OutputIterator _iter;
			lexy::visualization_options _opts;
			const char* _path;
			std::size_t _count;
			std::vector<ParseError> _errors;

			using return_type = std::vector<ParseError>;

			template<typename Input, typename Reader, typename Tag>
			void operator()(const lexy::error_context<Input>& context, const lexy::error<Reader, Tag>& error) {
				_iter = lexy_ext::_detail::write_error(_iter, context, error, _opts, _path);
				++_count;

				// Convert the context location and error location into line/column information.
				auto context_location = lexy::get_input_location(context.input(), context.position());
				auto location = lexy::get_input_location(context.input(), error.position(), context_location.anchor());

				std::string message;

				// Write the main annotation.
				if constexpr (std::is_same_v<Tag, lexy::expected_literal>) {
					auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

					message = std::string("expected '") + string.data() + "'";
				} else if constexpr (std::is_same_v<Tag, lexy::expected_keyword>) {
					auto string = lexy::_detail::make_literal_lexeme<typename Reader::encoding>(error.string(), error.length());

					message = std::string("expected keyword '") + string.data() + "'";
				} else if constexpr (std::is_same_v<Tag, lexy::expected_char_class>) {
					message = std::string("expected ") + error.name();
				} else {
					message = error.message();
				}

				_errors.push_back(
					ParseError {
						ParseError::Type::Fatal, // TODO: distinguish recoverable errors from fatal errors
						std::move(message),
						0, // TODO: implement proper error codes
						ParseData {
							context.production(),
							context_location.line_nr(),
							context_location.column_nr(),
						},
						location.line_nr(),
						location.column_nr(),
					});
			}

			return_type finish() && {
				if (_count != 0)
					*_iter++ = '\n';
				return _errors;
			}
		};
		constexpr auto sink() const {
			return _sink { _iter, _opts, _path, 0 };
		}

		/// Specifies a path that will be printed alongside the diagnostic.
		constexpr _ReportError path(const char* path) const {
			return { _iter, _opts, path };
		}

		/// Specifies an output iterator where the errors are written to.
		template<typename OI>
		constexpr _ReportError<OI> to(OI out) const {
			return { out, _opts, _path };
		}

		/// Overrides visualization options.
		constexpr _ReportError opts(lexy::visualization_options opts) const {
			return { _iter, opts, _path };
		}
	};

	constexpr auto ReporError = _ReportError<lexy::stderr_output_iterator> {};
}