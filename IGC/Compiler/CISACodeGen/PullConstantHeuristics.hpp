/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
#include <llvm/IR/InstVisitor.h>
#include "llvm/IR/Instruction.h"
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>

namespace IGC
{
    class PullConstantHeuristics : public llvm::ModulePass
    {
    public:
        PullConstantHeuristics() : llvm::ModulePass(ID)
        {
            initializePullConstantHeuristicsPass(*llvm::PassRegistry::getPassRegistry());
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::CodeGenContextWrapper>();
            AU.setPreservesAll();
        }

        bool runOnModule(llvm::Module& M) override;
        unsigned getPSDBottleNeckThreshold(const llvm::Function& F);

        unsigned getPushConstantThreshold(llvm::Function* F)
        {
            if (thresholdMap.find(F) != thresholdMap.end())
            {
                return thresholdMap[F];
            }

            const DWORD pushConstantGRFThreshold = IGC_GET_FLAG_VALUE(BlockPushConstantGRFThreshold);
            if (pushConstantGRFThreshold != 0xFFFFFFFF)
            {
                return pushConstantGRFThreshold;
            }
            else
            {
                const CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
                IGC_ASSERT_MESSAGE(ctx, "CodeGenContext not initialized.");
                return ctx->platform.getBlockPushConstantGRFThreshold();
            }

        }
        static char ID;
    private:
        std::map<llvm::Function*, unsigned> thresholdMap;
    };

}
