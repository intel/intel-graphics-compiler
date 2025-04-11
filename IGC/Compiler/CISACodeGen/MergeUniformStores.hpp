/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

using namespace llvm;

namespace IGC
{
    class MergeUniformStores : public FunctionPass, public InstVisitor<MergeUniformStores>
    {
    public:
        static char ID;

        MergeUniformStores();
        virtual bool runOnFunction(Function& F) override;
        virtual llvm::StringRef getPassName() const override
        {
            return "MergeUniformStores";
        }

        virtual void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<LoopInfoWrapperPass>();
            AU.addRequired<WIAnalysis>();
            AU.setPreservesAll();
        }

        void visitStoreInst(StoreInst& I);

    private:
        WIAnalysis* WI = nullptr;
        Function* mWaveBallotFn = nullptr;
        Function* mFirstBitHiFn = nullptr;
        Function* mWaveShuffleIndexFn = nullptr;
        CallInst* mWaveBallot = nullptr;
        CallInst* mFirstBitHi = nullptr;
        bool initialized = false;

        void doInitialization(StoreInst& I);
    };
} // namespace IGC
