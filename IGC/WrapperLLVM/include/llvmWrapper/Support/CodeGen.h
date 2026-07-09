/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_CODEGEN_H
#define IGCLLVM_SUPPORT_CODEGEN_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/CodeGen.h"

namespace IGCLLVM {

// LLVM 18 replaced the unscoped enum llvm::CodeGenOpt::Level with the scoped
// enum llvm::CodeGenOptLevel. The enumerator names (None, Less, Default,
// Aggressive) are identical, and unscoped enumerators may be accessed with the
// scoped syntax (e.g. CodeGenOpt::Level::None) since C++11, so IGCLLVM code can
// uniformly use IGCLLVM::CodeGenOptLevel and IGCLLVM::CodeGenOptLevel::<Name>.
#if LLVM_VERSION_MAJOR >= 18
using CodeGenOptLevel = llvm::CodeGenOptLevel;
#else
using CodeGenOptLevel = llvm::CodeGenOpt::Level;
#endif

// LLVM 18 turned CodeGenFileType into a scoped enum and dropped the CGFT_
// prefix from its enumerators.
#if LLVM_VERSION_MAJOR >= 18
inline constexpr llvm::CodeGenFileType CGFT_ObjectFile = llvm::CodeGenFileType::ObjectFile;
inline constexpr llvm::CodeGenFileType CGFT_AssemblyFile = llvm::CodeGenFileType::AssemblyFile;
#else
inline constexpr llvm::CodeGenFileType CGFT_ObjectFile = llvm::CGFT_ObjectFile;
inline constexpr llvm::CodeGenFileType CGFT_AssemblyFile = llvm::CGFT_AssemblyFile;
#endif

} // namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_CODEGEN_H
