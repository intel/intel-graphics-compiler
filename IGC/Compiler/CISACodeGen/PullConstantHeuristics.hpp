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

        // This function returns the push constant threshold in 32-byte units.
        unsigned getPushConstantThreshold(llvm::Function* F)
        {
            const CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
            if (thresholdMap.find(F) != thresholdMap.end())
            {
                return thresholdMap[F] * ctx->platform.getGRFSize() / (ctx->platform.getMinPushConstantBufferAlignment() * sizeof(DWORD));
            }

            DWORD pushConstantGRFThreshold = IGC_GET_FLAG_VALUE(BlockPushConstantGRFThreshold);
            if (pushConstantGRFThreshold != 0xFFFFFFFF)
            {
                return pushConstantGRFThreshold * ctx->platform.getGRFSize() / (ctx->platform.getMinPushConstantBufferAlignment() * sizeof(DWORD));
            }
            else
            {
                IGC_ASSERT_MESSAGE(ctx, "CodeGenContext not initialized.");
                return ctx->platform.getBlockPushConstantThreshold();
            }

        }
        static char ID;
    private:
        std::map<llvm::Function*, unsigned> thresholdMap;
    };

}
