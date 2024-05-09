#include "AbstractSyntaxTree.hpp"

using namespace ovdl;

AbstractSyntaxTree::symbol_type AbstractSyntaxTree::intern(const char* str, std::size_t length) {
	return _symbol_interner.intern(str, length);
}

AbstractSyntaxTree::symbol_type AbstractSyntaxTree::intern(std::string_view str) {
	return intern(str.data(), str.size());
}

const char* AbstractSyntaxTree::intern_cstr(const char* str, std::size_t length) {
	return intern(str, length).c_str(_symbol_interner);
}

const char* AbstractSyntaxTree::intern_cstr(std::string_view str) {
	return intern_cstr(str.data(), str.size());
}

AbstractSyntaxTree::symbol_interner_type& AbstractSyntaxTree::symbol_interner() {
	return _symbol_interner;
}

const AbstractSyntaxTree::symbol_interner_type& AbstractSyntaxTree::symbol_interner() const {
	return _symbol_interner;
}