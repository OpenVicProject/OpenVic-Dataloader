#pragma once

#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/ParseState.hpp>
#include <openvic-dataloader/v2script/AbstractSyntaxTree.hpp>

#include <lexy/encoding.hpp>

namespace ovdl::v2script::ast {
	using File = ovdl::BasicFile<lexy::default_encoding, Node>;
	struct AbstractSyntaxTree : ovdl::BasicAbstractSyntaxTree<File, FileTree> {
		using BasicAbstractSyntaxTree::BasicAbstractSyntaxTree;

		std::string make_list_visualizer() const;
		std::string make_native_visualizer() const;
	};

	using ParseState = ovdl::ParseState<AbstractSyntaxTree>;

	static_assert(IsFile<ast::File>, "File failed IsFile concept");
	static_assert(IsAst<ast::AbstractSyntaxTree>, "AbstractSyntaxTree failed IsAst concept");
	static_assert(IsParseState<ast::ParseState>, "ParseState failed IsParseState concept");
}