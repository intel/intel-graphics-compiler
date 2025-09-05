/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_INSTCOMBINE_INSTCOMBINE_H
#define IGC_INSTCOMBINE_INSTCOMBINE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/PassRegistry.h"
#include "llvmWrapper/Transforms/InstCombine/InstCombineWorklist.h"
#include "llvm/IR/PassManager.h"
#include "Compiler/InitializePasses.h"
#include "llvm/Transforms/InstCombine/InstCombine.h"

namespace IGC {
inline llvm::FunctionPass *createIGCInstructionCombiningPass() { return llvm::createInstructionCombiningPass(); }
} // namespace IGC

#endif // IGC_INSTCOMBINE_INSTCOMBINE_H
