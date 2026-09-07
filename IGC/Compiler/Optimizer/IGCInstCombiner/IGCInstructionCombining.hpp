/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_INSTCOMBINE_INSTCOMBINE_H
#define IGC_INSTCOMBINE_INSTCOMBINE_H

#include "llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h"
#include "Compiler/InitializePasses.h"

#include "LLVMWarningsPush.hpp"
#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "LLVMWarningsPop.hpp"

namespace IGC {
inline llvm::FunctionPass *createIGCInstructionCombiningPass() {
  llvm::initializeInstructionCombiningPassPass(*llvm::PassRegistry::getPassRegistry());
  return llvm::createInstructionCombiningPass();
}
} // namespace IGC

#endif // IGC_INSTCOMBINE_INSTCOMBINE_H
