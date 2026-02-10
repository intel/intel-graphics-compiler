/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_STRINGREF_H
#define IGCLLVM_ADT_STRINGREF_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/StringRef.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline bool equals_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) { return LHS.equals_insensitive(RHS); }

inline bool ends_with_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) {
#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
  return LHS.ends_with_insensitive(RHS);
#else
  return LHS.endswith_insensitive(RHS);
#endif
}

inline bool contains_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) { return LHS.contains_insensitive(RHS); }
} // namespace IGCLLVM

#endif
