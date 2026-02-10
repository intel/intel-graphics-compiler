/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_VALUETRACKING_H
#define IGCLLVM_ANALYSIS_VALUETRACKING_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/Analysis/ValueTracking.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::Value *getUnderlyingObject(llvm::Value *V, const llvm::DataLayout &DL) {
  (void)DL;
  return llvm::getUnderlyingObject(V);
}
} // namespace IGCLLVM

#endif
