/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_ALWAYSINLINER_H
#define IGCLLVM_TRANSFORMS_IPO_ALWAYSINLINER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/IPO/AlwaysInliner.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

inline llvm::Pass *createAlwaysInlinerLegacyPass(bool InsertLifetime = true) {
  llvm::initializeAlwaysInlinerLegacyPassPass(*llvm::PassRegistry::getPassRegistry());
  return llvm::createAlwaysInlinerLegacyPass(InsertLifetime);
}

} // namespace IGCLLVM

#endif
