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

#if LLVM_VERSION_MAJOR <= 7
namespace llvm {
    class FunctionPass;
}
#elif LLVM_VERSION_MAJOR >= 8
#include "llvm/Transforms/InstCombine/InstCombine.h"
#endif

namespace IGC
{
#if LLVM_VERSION_MAJOR <= 7
    class IGCInstructionCombiningPass : public llvm::FunctionPass {
        llvm::InstCombineWorklist Worklist;
        const bool ExpensiveCombines;

    public:
        static char ID; // Pass identification, replacement for typeid

        IGCInstructionCombiningPass(bool ExpensiveCombines = true)
            : FunctionPass(ID), ExpensiveCombines(ExpensiveCombines) {
            initializeIGCInstructionCombiningPassPass(*llvm::PassRegistry::getPassRegistry());
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
        bool runOnFunction(llvm::Function &F) override;
    };


    llvm::FunctionPass* createIGCInstructionCombiningPass();
#elif LLVM_VERSION_MAJOR <= 10
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

