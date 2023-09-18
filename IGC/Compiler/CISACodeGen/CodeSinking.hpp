/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    class CodeSinking : public llvm::FunctionPass {
        llvm::DominatorTree* DT;
        llvm::PostDominatorTree* PDT;
        llvm::LoopInfo* LI;
        llvm::AliasAnalysis* AA;
        WIAnalysis* WI;

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
            AU.addRequired<llvm::AAResultsWrapperPass>();
            AU.addRequired<WIAnalysis>();
            AU.addRequired<CodeGenContextWrapper>();

            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
            AU.addPreserved<llvm::AAResultsWrapperPass>();
            AU.addPreservedID(WIAnalysis::ID);
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
        bool isSafeToLoopSinkLoad(llvm::Instruction* I, llvm::Loop* Loop, llvm::AliasAnalysis* AA);
        bool isAlwaysSinkInstruction(llvm::Instruction* I);

        /// local processing
        bool LocalSink(llvm::BasicBlock* blk);
        /// data members for local-sinking
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> LocalBlkSet;
        llvm::SmallPtrSet<llvm::Instruction*, 8> LocalInstSet;
        /// data members for undo
        std::vector<llvm::Instruction*> movedInsts;
        std::vector<llvm::Instruction*> undoLocas;
        /// counting the number of gradient/sample operation sinked into CF
        unsigned totalGradientMoved;
        unsigned numGradientMovedOutBB;

        bool generalCodeSinking;
        // diagnosis variable: int numChanges;

        // Keep track of fat loop. Might need to reverse LICM if
        // a loop has excessive register-pressure at its preheader because
        // there are a lot of loop-invariant insts that have been moved
        // out of the loop.
        std::vector<llvm::Loop*> m_fatLoops;
        std::vector<uint32_t> m_fatLoopPressures;

        // try to hoist phi nodes with congruent incoming values
        typedef std::pair<llvm::Instruction*, llvm::Instruction*> InstPair;
        typedef smallvector<llvm::Instruction*, 4> InstVec;

        // memoize all possible stores for every loop that is a candidate for sinking
        typedef llvm::SmallVector<llvm::Instruction*, 32> StoresVec;
        llvm::DenseMap<llvm::Loop*, StoresVec> MemoizedStoresInLoops;
        llvm::SmallPtrSet<llvm::Loop*, 8> BlacklistedLoops;
        const StoresVec getAllStoresInLoop(llvm::Loop* L);

        void appendIfNotExist(InstPair src, std::vector<InstPair> &instMap)
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
        bool checkCongruent(std::vector<InstPair> &instMap, const InstPair& values, InstVec& leaves, unsigned depth);

        /**
         * Detech phi with congruent incoming values, and try to hoist them to
         * dominator.  In some cases, GVN may leave code like this and increase
         * register pressure.
         */
        bool hoistCongruentPhi(llvm::PHINode* phi);
        bool hoistCongruentPhi(llvm::Function& F);

        llvm::Loop* findLoopAsPreheader(llvm::BasicBlock& blk);
        // move LI back into loop
        bool loopSink(llvm::Loop* LoopWithPressure);
        // pre-condition to sink an instruction into a loop
        bool isLoopSinkCandidate(llvm::Instruction* I, llvm::Loop* L);
        bool loopSinkInstructions(
            llvm::SmallVector<llvm::Instruction*, 64>& SinkCandidates,
            llvm::SmallPtrSet<llvm::Instruction*, 32>& LoadChains,
            llvm::Loop* L);

        // Move referencing DbgValueInst intrinsics calls after defining instructions
        void ProcessDbgValueInst(llvm::BasicBlock& blk);

    };
    void initializeCodeSinkingPass(llvm::PassRegistry&);

}

