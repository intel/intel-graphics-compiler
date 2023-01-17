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

namespace IGC
{
#if LLVM_VERSION_MAJOR <= 10
    inline llvm::FunctionPass* createIGCInstructionCombiningPass()
    {
        return llvm::createInstructionCombiningPass(false);
    }
#else
    inline llvm::FunctionPass* createIGCInstructionCombiningPass()
    {
        return llvm::createInstructionCombiningPass();
    }
#endif
} // namespace IGC

#endif //IGC_INSTCOMBINE_INSTCOMBINE_H

