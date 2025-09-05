/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_STRINGREF_H
#define IGCLLVM_ADT_STRINGREF_H

#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/StringRef.h"

namespace IGCLLVM {
inline bool equals_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) {
  return LHS.equals_insensitive(RHS);
}

inline bool endswith_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) {
  return LHS.endswith_insensitive(RHS);
}

inline bool contains_insensitive(llvm::StringRef LHS, llvm::StringRef RHS) {
  return LHS.contains_insensitive(RHS);
}
} // namespace IGCLLVM

#endif
