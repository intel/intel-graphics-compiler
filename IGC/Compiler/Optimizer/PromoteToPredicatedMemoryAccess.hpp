/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

using namespace llvm;

namespace IGC
{
    // This pass replaces LLVM IR load/store
    //     %17 = load <4 x float>, <4 x float> addrspace(1)* %bitc0, align 16
    // with GenISA predicated load/store intrinsic
    //     %17 = call <4 x float> @llvm.genx.GenISA.PredicatedLoad.v4f32.p1v4f32.v4f32(<4 x float> addrspace(1)* %bitc0, i64 16, i1 %pred, <4 x float> %mergeValue)
    // if found in specific pattern and then performs if-conversion.
    //
    // Constraints:
    // 1. The pass looks for conditional branches that can be if-converted. The only "hammock" form
    // of the control flow is supported, i.e. the true block has a single
    // predecessor and the false block has two predecessors. The true block must
    // have a single successor that is the false block.
    //
    //   if (cmp) {
    //     true block
    //   }
    //   false block
    //
    // 2. All the instructions in the true block must be safe to execute in the false
    // block. The pass makes the instructions in the true block to be executed
    // conditionally and replaces branch to unconditional one.
    // 3. Load/Store instructions should be simple.
    // 4. Load/Store instructions should use legal data types.
    //
    // The pass expects that the simplifycfg pass will be run after it to clean up
    // the CFG.

    class PromoteToPredicatedMemoryAccess : public FunctionPass
    {
    public:
        static char ID;
        explicit PromoteToPredicatedMemoryAccess();
        ~PromoteToPredicatedMemoryAccess() {}

        StringRef getPassName() const override
        {
            return "PromoteToPredicatedMemoryAccess";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        bool runOnFunction(Function &F) override;

    private:
        bool trySingleBlockIfConv(Value &Cond, BasicBlock &BranchBB,
                                  BasicBlock &ConvBB, BasicBlock &SuccBB,
                                  bool Inverse = false);

        void convertMemoryAccesses(Instruction *Mem, Value *MergeV, Value *Cond,
                                   bool Inverse);

        void fixPhiNode(PHINode &Phi, BasicBlock &Predecessor, BasicBlock &CondBlock);
    };
} // namespace IGC
