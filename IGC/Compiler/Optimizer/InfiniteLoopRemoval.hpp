/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

    class InfiniteLoopRemoval :
        public llvm::FunctionPass
    {
    public:
        static char ID;

        InfiniteLoopRemoval();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
        }

        virtual llvm::StringRef getPassName() const override
        {
            return "InfiniteLoopRemoval";
        }
    };

} // namespace
