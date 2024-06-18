#pragma once

#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/encoding.hpp>

#include "../openvic-dataloader/ParseState.hpp"
#include "AbstractSyntaxTree.hpp"
#include "File.hpp"
#include "detail/InternalConcepts.hpp"

namespace ovdl::v2script::ast {

	struct FileAbstractSyntaxTree : ovdl::BasicAbstractSyntaxTree<ovdl::BasicFile<Node>, FileTree> {
		using ovdl::BasicAbstractSyntaxTree<ovdl::BasicFile<Node>, FileTree>::BasicAbstractSyntaxTree;

		std::string make_list_visualizer() const;
		std::string make_native_visualizer() const;
	};

	using ParseState = ovdl::ParseState<FileAbstractSyntaxTree>;

	static_assert(detail::IsParseState<ast::ParseState>, "ParseState failed IsParseState concept");
}