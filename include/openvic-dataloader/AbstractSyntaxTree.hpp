#pragma once

#include <concepts>
#include <cstdio>
#include <string_view>
#include <utility>

#include <openvic-dataloader/File.hpp>
#include <openvic-dataloader/NodeLocation.hpp>
#include <openvic-dataloader/detail/SymbolIntern.hpp>
#include <openvic-dataloader/detail/utility/Utility.hpp>

#include <dryad/node.hpp>
#include <dryad/node_map.hpp>
#include <dryad/symbol.hpp>
#include <dryad/tree.hpp>

#include <fmt/core.h>

namespace ovdl {
	struct AbstractSyntaxTree : SymbolIntern {
		symbol_type intern(const char* str, std::size_t length);
		symbol_type intern(std::string_view str);
		const char* intern_cstr(const char* str, std::size_t length);
		const char* intern_cstr(std::string_view str);
		symbol_interner_type& symbol_interner();
		const symbol_interner_type& symbol_interner() const;

	protected:
		symbol_interner_type _symbol_interner;
	};

	template<typename T>
	concept IsAst =
		std::derived_from<T, AbstractSyntaxTree> &&
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

	template<IsFile FileT, std::derived_from<typename FileT::node_type> RootNodeT>
	struct BasicAbstractSyntaxTree : AbstractSyntaxTree {
		using file_type = FileT;
		using root_node_type = RootNodeT;
		using node_type = typename file_type::node_type;

		explicit BasicAbstractSyntaxTree(file_type&& file) : _file(std::move(file)) {}
		explicit BasicAbstractSyntaxTree(lexy::buffer<typename file_type::encoding_type, void>&& buffer) : _file(std::move(buffer)) {}

		void set_location(const node_type* n, NodeLocation loc) {
			_file.set_location(n, loc);
		}

		NodeLocation location_of(const node_type* n) const {
			return _file.location_of(n);
		}

		root_node_type* root() {
			return _tree.root();
		}

		const root_node_type* root() const {
			return _tree.root();
		}

		file_type& file() {
			return _file;
		}

		const file_type& file() const {
			return _file;
		}

		template<typename T, typename... Args>
		T* create(NodeLocation loc, Args&&... args) {
			auto node = _tree.template create<T>(DRYAD_FWD(args)...);
			set_location(node, loc);
			return node;
		}

		template<typename T, typename... Args>
		T* create(const char* begin, const char* end, Args&&... args) {
			return create<T>(NodeLocation::make_from(begin, end), DRYAD_FWD(args)...);
		}

		void set_root(root_node_type* node) {
			_tree.set_root(node);
		}

	protected:
		dryad::tree<root_node_type> _tree;
		file_type _file;
	};
}