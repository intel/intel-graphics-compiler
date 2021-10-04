/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_IGCDEPS_SRC_LEGACY_INFO_GENERATION_H
#define VC_IGCDEPS_SRC_LEGACY_INFO_GENERATION_H

#include "vc/GenXCodeGen/GenXOCLRuntimeInfo.h"

#include <tuple>

namespace vc {

// Emits legacy symbol table based on the provided zebin symbol tables.
// Module symbols consist of constant and global symbols.
// Returns pointer to the buffer with the symbols, the size of the buffer and
// the number of entries in the buffer respectively. The buffer is allocated
// via std::malloc.
std::tuple<void *, unsigned, unsigned> emitLegacyModuleSymbolTable(
    const llvm::GenXOCLRuntimeInfo::SymbolSeq &Constants,
    const llvm::GenXOCLRuntimeInfo::SymbolSeq &Globals);

// Validates zebin function symbol table. Kernel symbols should go first, after
// them should go other function symbols. Curently there cannot be more than
// 1 kernel symbol. It is possible to have no kernel symbols in the table.
void validateFunctionSymbolTable(
    const llvm::GenXOCLRuntimeInfo::SymbolSeq &Funcs);

// Emits legacy symbol table based on the provided zebin symbol table.
// This function will skip kernel symbols (curently there cannot be more than
// 1 kernel symbol) as a legacy symbol table must not contain a kernel symbol.
// Returns pointer to the buffer with the symbols, the size of the buffer and
// the number of entries in the buffer respectively. The buffer is allocated
// via std::malloc.
std::tuple<void *, unsigned, unsigned>
emitLegacyFunctionSymbolTable(const llvm::GenXOCLRuntimeInfo::SymbolSeq &Funcs);

} // namespace vc

#endif // VC_IGCDEPS_SRC_LEGACY_INFO_GENERATION_H
