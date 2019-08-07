/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
/**
 * Originated from llvm code-sinking, need add their copyright
 **/
 //===-- Sink.cpp - Code Sinking -------------------------------------------===//
 //
 //                     The LLVM Compiler Infrastructure
 //
 // This file is distributed under the University of Illinois Open Source
 // License. See LICENSE.TXT for details.
 //
 //===----------------------------------------------------------------------===//

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

#define CODE_SINKING_MIN_SIZE  32

    class CodeSinking : public llvm::FunctionPass {
        llvm::DominatorTree* DT;
        llvm::PostDominatorTree* PDT;
        llvm::LoopInfo* LI;
        const llvm::DataLayout* DL;  // to estimate register pressure
        CodeGenContext* CTX;
    public:
        static char ID; // Pass identification

        CodeSinking(bool generalSinking = false);

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
        }
    private:
        bool ProcessBlock(llvm::BasicBlock& blk);
        bool SinkInstruction(llvm::Instruction* I,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& Stores,
            bool ForceToReducePressure);
        bool AllUsesDominatedByBlock(llvm::Instruction* inst,
            llvm::BasicBlock* blk,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& usesInBlk) const;
        bool FindLowestSinkTarget(llvm::Instruction* inst,
            llvm::BasicBlock*& blk,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& usesInBlk, bool& outerLoop,
            bool doLoopSink);
        bool isSafeToMove(llvm::Instruction* inst,
            bool& reducePressure, bool& hasAliasConcern,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& Stores);

        /// local processing
        bool LocalSink(llvm::BasicBlock* blk);
        /// data members for local-sinking
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> localBlkSet;
        llvm::SmallPtrSet<llvm::Instruction*, 8> localInstSet;
        /// data members for undo
        std::vector<llvm::Instruction*> movedInsts;
        std::vector<llvm::Instruction*> undoLocas;
        /// counting the number of gradient/sample operation sinked into CF
        unsigned totalGradientMoved;
        unsigned numGradientMovedOutBB;

        bool generalCodeSinking;
        // diagnosis variable: int numChanges;

        // fat BB is the BB with the largest register pressure
        // Currently, used it for BB inside a loop only.
        llvm::BasicBlock* m_fatBB;
        uint32_t m_fatBBPressure;

        // try to hoist phi nodes with congruent incoming values
        typedef std::pair<llvm::Instruction*, llvm::Instruction*> InstPair;
        typedef smallvector<llvm::Instruction*, 4> InstVec;

        // this vector maps all instructions leading to source0 of phi instruction to
        // the corresponding instructions of source1
        std::vector<InstPair> instMap;

        void appendIfNotExist(InstPair src)
        {
            if (std::find(instMap.begin(), instMap.end(), src) == instMap.end())
            {
                instMap.push_back(src);
            }
        }
        void appendIfNotExist(InstVec& dst, llvm::Instruction* inst)
        {
            if (std::find(dst.begin(), dst.end(), inst) == dst.end())
            {
                dst.push_back(inst);
            }
        }
        void appendIfNotExist(InstVec& dst, InstVec& src)
        {
            for (auto* I : src)
            {
                appendIfNotExist(dst, I);
            }
        }

        // check if two values are congruent (derived from same values), and
        // record all intermediate results in vector.
        bool checkCongruent(const InstPair& values, InstVec& leaves, unsigned depth);

        /**
         * Detech phi with congruent incoming values, and try to hoist them to
         * dominator.  In some cases, GVN may leave code like this and increase
         * register pressure.
         */
        bool hoistCongruentPhi(llvm::PHINode* phi);
        bool hoistCongruentPhi(llvm::Function& F);

        // Move LI back into loops
        bool loopSink(llvm::BasicBlock* BBWithPressure, bool SinkMultipleLevel);
        bool canLoopSink(llvm::Instruction* I, llvm::Loop* L, llvm::BasicBlock* BB);
        bool LoopSinkInstructions(
            llvm::SmallVector<llvm::Instruction*, 64> sinkCandidates, llvm::Loop* L);
    };

}

