/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CShader;

    /// @brief sort basic blocks into topological order
    /// Arbitrary reverse postorder is not sufficient.
    /// Whenever it is possible, we want to layout blocks in such way
    /// that the vIsa Jitter can recognize the control-flow structures
    class Layout : public llvm::FunctionPass
    {
    public:
        static char ID;
        Layout();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override {
            return "Layout";
        }

        virtual bool runOnFunction(llvm::Function& func) override;

    private:
        llvm::BasicBlock* getLastReturnBlock(llvm::Function& Func);
        void LayoutBlocks(llvm::Function& func, llvm::LoopInfo& LI);
        void LayoutBlocks(llvm::Function& func);
        llvm::BasicBlock* selectSucc(
            llvm::BasicBlock* CurrBlk,
            bool SelectNoInstBlk,
            const llvm::LoopInfo& LI,
            const std::set<llvm::BasicBlock*>& VisitSet);

        bool isAtomicWrite(llvm::Instruction* inst, bool onlyLocalMem);
        bool isAtomicRead(llvm::Instruction* inst, bool onlyLocalMem);
        llvm::Value* getMemoryOperand(llvm::Instruction* inst, bool onlyLocalMem);
        bool isReturnBlock(llvm::BasicBlock* bb);
        bool tryMovingWrite(llvm::Instruction* write, llvm::Loop* loop, llvm::LoopInfo& LI);
        void moveAtomicWrites2Loop(llvm::Function& func, llvm::LoopInfo& LI, bool onlyLocalMem);

        llvm::PostDominatorTree* m_PDT;
        llvm::DominatorTree* m_DT;
    };

}
