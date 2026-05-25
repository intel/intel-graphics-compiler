/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DEBUGINFO_H
#define IGCLLVM_IR_DEBUGINFO_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/DebugInfo.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

inline auto findDbgDeclareUses(llvm::Value *V) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::findDVRDeclares(V);
#else
  return llvm::FindDbgDeclareUses(V);
#endif
}
} // namespace IGCLLVM

#endif
