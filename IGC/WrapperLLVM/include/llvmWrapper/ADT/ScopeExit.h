/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_SCOPEEXIT_H
#define IGCLLVM_ADT_SCOPEEXIT_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/ADT/ScopeExit.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
template <typename Callable> auto make_scope_exit(Callable &&F) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::make_scope_exit(std::forward<Callable>(F));
#else
  return llvm::scope_exit(std::forward<Callable>(F));
#endif
}
} // namespace IGCLLVM

#endif
