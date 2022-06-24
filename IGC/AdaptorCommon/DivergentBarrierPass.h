/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "IGC/common/StringMacros.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class DivergentBarrierPass : public llvm::ModulePass
    {
    public:
        DivergentBarrierPass(void* Ctx = nullptr) :
            llvm::ModulePass(ID), Ctx(Ctx) {}
        bool runOnModule(llvm::Module& M) override;

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        static char ID;

        llvm::StringRef getPassName() const override
        {
            return "DivergentBarrierPass";
        }
    private:
        struct FenceArgs
        {
            bool CommitEnable           = true;
            bool L3_Flush_RW_Data       = false;
            bool L3_Flush_Constant_Data = false;
            bool L3_Flush_Texture_Data  = false;
            bool L3_Flush_Instructions  = false;
            bool Global                 = false;
            bool L1_Invalidate          = false;
        };

        CodeGenContext* m_CGCtx = nullptr;
        IGCMD::MetaDataUtils* m_MDUtils = nullptr;
        bool processShader(llvm::Function* F);
        bool hasDivergentBarrier(
            const std::vector<llvm::Instruction*>& Barriers) const;
        llvm::Function* createContinuation(llvm::BasicBlock* EntryBB);
        void updateFenceArgs(
            const llvm::GenIntrinsicInst* I, FenceArgs& Args) const;
        void generateBody(
            llvm::Function* Wrapper,
            llvm::Function* Entry,
            const std::vector<llvm::Function*>& Continuations,
            const FenceArgs &FA);
        llvm::Value* getGroupSize(llvm::Function& F) const;
        llvm::Value* allocateSLM(llvm::IRBuilder<> &IRB);
        llvm::CallInst* insertFence(llvm::IRBuilder<>& IRB, const FenceArgs& FA) const;
        void handleSpillFill(llvm::Function* F) const;

        void* Ctx = nullptr;
    };

    void initializeDivergentBarrierPassPass(llvm::PassRegistry&);
    llvm::ModulePass* createDivergentBarrierPass(void* Ctx);
}//namespace IGC
