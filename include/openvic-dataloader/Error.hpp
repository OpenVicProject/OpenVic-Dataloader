#pragma once

#include <cstdint>
#include <string_view>

#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/detail/utility/Utility.hpp>

#include <dryad/abstract_node.hpp>
#include <dryad/node.hpp>

namespace ovdl {
	template<IsFile>
	struct BasicDiagnosticLogger;
}

namespace ovdl::error {
	enum class [[nodiscard]] ErrorKind : std::uint64_t {
		Root,

		BufferError,

		// Parse Error //
		ExpectedLiteral,
		ExpectedKeyword,
		ExpectedCharClass,
		GenericParseError,

		FirstParseError = ExpectedLiteral,
		LastParseError = GenericParseError,

		// Semantic Diagnostic //
		SemanticError,
		SemanticWarning,
		SemanticInfo,
		SemanticDebug,
		SemanticFixit,
		SemanticHelp,

		FirstSemantic = SemanticError,
		LastSemantic = SemanticHelp,

		PrimaryAnnotation,
		SecondaryAnnotation,

		FirstAnnotation = PrimaryAnnotation,
		LastAnnotation = SecondaryAnnotation,
	};

	static constexpr std::string_view get_kind_name(ErrorKind kind) {
		switch (kind) {
			using enum ErrorKind;
			case ExpectedLiteral: return "expected literal";
			case ExpectedKeyword: return "expected keyword";
			case ExpectedCharClass: return "expected char class";
			case GenericParseError: return "generic";
			default: detail::unreachable();
		}
	}

	struct Error : dryad::abstract_node_all<ErrorKind> {
		std::string_view message() const { return _message; }

	protected:
		DRYAD_ABSTRACT_NODE_CTOR(Error);

		void _set_message(const char* message) { _message = message; }
		const char* _message;

		template<IsFile>
		friend struct ovdl::BasicDiagnosticLogger;
	};

	using ErrorList = dryad::unlinked_node_list<Error>;

	struct Annotation;
	using AnnotationList = dryad::unlinked_node_list<Annotation>;

	struct Root : dryad::basic_node<ErrorKind::Root, dryad::container_node<Error>> {
		explicit Root(dryad::node_ctor ctor) : node_base(ctor) {}

		DRYAD_CHILD_NODE_RANGE_GETTER(Error, errors, nullptr, this->node_after(_last));

		void insert_back(Error* error) {
			insert_child_after(_last, error);
			_last = error;
		}

	private:
		Error* _last = nullptr;
	};

	struct BufferError : dryad::basic_node<ErrorKind::BufferError, Error> {
		explicit BufferError(dryad::node_ctor ctor, const char* message) : node_base(ctor) {
			_set_message(message);
		}

		explicit BufferError(dryad::node_ctor ctor) : node_base(ctor) {}
	};

	struct ParseError : dryad::abstract_node_range<Error, ErrorKind::FirstParseError, ErrorKind::LastParseError> {
		std::string_view production_name() const { return _production_name; }

	protected:
		explicit ParseError(dryad::node_ctor ctor,
			ErrorKind kind,
			const char* message,
			const char* production_name)
			: node_base(ctor, kind),
			  _production_name(production_name) {
			_set_message(message);
		};

		const char* _production_name;
	};

	template<ErrorKind NodeKind>
	struct _ParseError_t : dryad::basic_node<NodeKind, ParseError> {
		explicit _ParseError_t(dryad::node_ctor ctor, const char* message, const char* production_name)
			: dryad::basic_node<NodeKind, ParseError>(ctor, message, production_name) {}
	};

	using ExpectedLiteral = _ParseError_t<ErrorKind::ExpectedLiteral>;
	using ExpectedKeyword = _ParseError_t<ErrorKind::ExpectedKeyword>;
	using ExpectedCharClass = _ParseError_t<ErrorKind::ExpectedCharClass>;
	using GenericParseError = _ParseError_t<ErrorKind::GenericParseError>;

	struct Semantic : dryad::abstract_node_range<dryad::container_node<Error>, ErrorKind::FirstSemantic, ErrorKind::LastSemantic> {
		DRYAD_CHILD_NODE_RANGE_GETTER(Annotation, annotations, nullptr, this->node_after(_last_annotation));

		void push_back(Annotation* annotation);
		void push_back(AnnotationList p_annotations);

	protected:
		explicit Semantic(dryad::node_ctor ctor, ErrorKind kind)
			: node_base(ctor, kind) {};

		explicit Semantic(dryad::node_ctor ctor, ErrorKind kind, const char* message)
			: node_base(ctor, kind) {
			insert_child_list_after(nullptr, AnnotationList {});
			_set_message(message);
		};

		explicit Semantic(dryad::node_ctor ctor, ErrorKind kind, const char* message, AnnotationList annotations)
			: node_base(ctor, kind) {
			insert_child_list_after(nullptr, annotations);
			_set_message(message);
		};

	private:
		Error* _last_annotation;
	};

	template<ErrorKind NodeKind>
	struct _SemanticError_t : dryad::basic_node<NodeKind, Semantic> {
		using base_node = dryad::basic_node<NodeKind, Semantic>;

		explicit _SemanticError_t(dryad::node_ctor ctor)
			: base_node(ctor) {}

		explicit _SemanticError_t(dryad::node_ctor ctor, const char* message)
			: base_node(ctor, message) {}

		explicit _SemanticError_t(dryad::node_ctor ctor, const char* message, AnnotationList annotations)
			: base_node(ctor, message, annotations) {}
	};

	using SemanticError = _SemanticError_t<ErrorKind::SemanticError>;
	using SemanticWarning = _SemanticError_t<ErrorKind::SemanticWarning>;
	using SemanticInfo = _SemanticError_t<ErrorKind::SemanticInfo>;
	using SemanticDebug = _SemanticError_t<ErrorKind::SemanticDebug>;
	using SemanticFixit = _SemanticError_t<ErrorKind::SemanticFixit>;
	using SemanticHelp = _SemanticError_t<ErrorKind::SemanticHelp>;

	struct Annotation : dryad::abstract_node_range<Error, ErrorKind::FirstAnnotation, ErrorKind::LastAnnotation> {
	protected:
		explicit Annotation(dryad::node_ctor ctor, ErrorKind kind, const char* message) : node_base(ctor, kind) {
			_set_message(message);
		}
	};

	template<ErrorKind NodeKind>
	struct _Annotation_t : dryad::basic_node<NodeKind, Annotation> {
		explicit _Annotation_t(dryad::node_ctor ctor, const char* message)
			: dryad::basic_node<NodeKind, Annotation>(ctor, message) {}
	};

	using PrimaryAnnotation = _Annotation_t<ErrorKind::PrimaryAnnotation>;
	using SecondaryAnnotation = _Annotation_t<ErrorKind::SecondaryAnnotation>;

	inline void Semantic::push_back(Annotation* annotation) {
		insert_child_after(annotations().end().deref(), annotation);
		_last_annotation = annotation;
	}

	inline void Semantic::push_back(AnnotationList p_annotations) {
		insert_child_list_after(annotations().end().deref(), p_annotations);
		_last_annotation = *p_annotations.end();
	}
}