#pragma once

#include <cstdint>

#include <dryad/symbol.hpp>

namespace ovdl {
	struct SymbolIntern {
		struct SymbolId;
		using index_type = std::uint32_t;
		using symbol_type = dryad::symbol<SymbolId, index_type>;
		using symbol_interner_type = dryad::symbol_interner<SymbolId, char, index_type>;
	};
}