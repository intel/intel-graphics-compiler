/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
class MatchCommonKernelPatterns : public llvm::FunctionPass
    {
    public:
        static char ID;

        MatchCommonKernelPatterns();

        virtual llvm::StringRef getPassName() const override {
            return "Match common kernel patterns";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesAll();
        }

        virtual bool runOnFunction(llvm::Function &F) override;
    private:
        bool isInterpreterPattern(const llvm::Function &F) const;
        bool isBBPartOfInterpreterPattern(const llvm::BasicBlock *BB) const;
        bool isBBBackEdge(const llvm::BasicBlock *BB, const llvm::BasicBlock *EntryBB) const;
        llvm::Value *getInterpreterSwitchArg(const llvm::BasicBlock *BB) const;

        CodeGenContext *Ctx = nullptr;
    };
}