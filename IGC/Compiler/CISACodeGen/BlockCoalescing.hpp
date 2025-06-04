/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/DeSSA.hpp"
#include "Compiler/CISACodeGen/PatternMatchPass.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CodeGenContextWrapper;

    class BlockCoalescing : public llvm::FunctionPass
    {
    public:
        static char ID;

        BlockCoalescing();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesAll();
            AU.addRequired<DeSSA>();
            AU.addRequired<CodeGenPatternMatch>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override {
            return "BlockCoalescing";
        }
        bool IsEmptyBlock(llvm::BasicBlock* bb);
        /// look for the next none-empty basick block
        llvm::BasicBlock* SkipEmptyBasicBlock(llvm::BasicBlock* bb);
        /// look for the destination to jump to if the block is empty
        llvm::BasicBlock* FollowEmptyBlock(llvm::BasicBlock* bb);
    protected:
        llvm::DenseSet<llvm::BasicBlock*> m_emptyBlocks;

    private:
        bool hasEmptyBlockLoop(llvm::BasicBlock* EmptyBlock);
    };
}
