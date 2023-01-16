/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class IndirectCallOptimization : public llvm::FunctionPass
    {
    public:
        static char ID;

        IndirectCallOptimization();
        ~IndirectCallOptimization() {}

        llvm::StringRef getPassName() const override
        {
            return "IndirectCallOptimization";
        }

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        bool runOnFunction(llvm::Function &F) override;

        bool visitCallInst(llvm::CallInst &CI);

    private:
    };
    void initializeIndirectCallOptimizationPass(llvm::PassRegistry&);
} // namespace IGC
