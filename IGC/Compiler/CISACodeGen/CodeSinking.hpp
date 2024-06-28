/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
    class CodeSinking : public llvm::FunctionPass {
        llvm::DominatorTree* DT = nullptr;
        llvm::PostDominatorTree* PDT = nullptr;
        llvm::LoopInfo* LI = nullptr;
        const llvm::DataLayout* DL = nullptr;  // to estimate register pressure
        CodeGenContext* CTX = nullptr;
    public:
        static char ID; // Pass identification

        CodeSinking();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();

            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();

            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
        }
    private:

        bool treeSink(llvm::Function& F);
        bool processBlock(llvm::BasicBlock& blk);
        bool sinkInstruction(llvm::Instruction* I,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& Stores);
        bool allUsesDominatedByBlock(llvm::Instruction* inst,
            llvm::BasicBlock* blk,
            llvm::SmallPtrSetImpl<llvm::Instruction*>& usesInBlk) const;
        bool isSafeToMove(llvm::Instruction* inst,
            bool& reducePressure,
            bool& hasAliasConcern,
            SmallPtrSetImpl<Instruction*>& Stores);

        uint estimateLiveOutPressure(llvm::BasicBlock* blk, const llvm::DataLayout* DL);

        /// data members for local-sinking
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> LocalBlkSet;
        llvm::SmallPtrSet<llvm::Instruction*, 8> LocalInstSet;
        /// data members for undo
        std::vector<llvm::Instruction*> MovedInsts;
        std::vector<llvm::Instruction*> UndoLocas;
        /// counting the number of gradient/sample operation sinked into CF
        unsigned totalGradientMoved = 0;
        unsigned numGradientMovedOutBB = 0;
    };

    void initializeCodeSinkingPass(llvm::PassRegistry&);


    typedef enum { NoSink=0, SinkWhileRegpressureIsHigh, FullSink } LoopSinkMode;

    class CodeLoopSinking : public llvm::FunctionPass {
        llvm::DominatorTree* DT = nullptr;
        llvm::PostDominatorTree* PDT = nullptr;
        llvm::LoopInfo* LI = nullptr;
        llvm::AliasAnalysis* AA = nullptr;
        WIAnalysisRunner WI;
        IGCMD::MetaDataUtils* MDUtils = nullptr;
        ModuleMetaData* ModMD = nullptr;
        IGCLivenessAnalysis* RPE = nullptr;
        IGCFunctionExternalRegPressureAnalysis* FRPE = nullptr;
        CodeGenContext* CTX = nullptr;

    public:
        static char ID; // Pass identification

        CodeLoopSinking();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();

            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<llvm::AAResultsWrapperPass>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<TranslationTable>();
            AU.addRequired<IGCLivenessAnalysis>();
            AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
            AU.addRequired<CodeGenContextWrapper>();

            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
            AU.addPreserved<llvm::AAResultsWrapperPass>();
            AU.addPreservedID(TranslationTable::ID);
        }
    private:

        /// sinking
        bool loopSink(llvm::Function& F);
        bool loopSink(llvm::Loop* LoopWithPressure, LoopSinkMode Mode);

        bool loopSinkInstructions(
            llvm::SmallVector<llvm::Instruction*, 64>& SinkCandidates,
            llvm::SmallPtrSet<llvm::Instruction*, 32>& LoadChains,
            llvm::Loop* L);

        bool isSafeToLoopSinkLoad(llvm::Instruction* I, llvm::Loop* Loop);
        bool isAlwaysSinkInstruction(llvm::Instruction* I);
        bool sinkInstruction(llvm::Instruction* I);

        // pre-condition to sink an instruction into a loop
        bool isLoopSinkCandidate(llvm::Instruction* I, llvm::Loop* L, bool AllowLoadSinking);
        bool isLoadChain(llvm::Instruction* I, SmallPtrSet<Instruction*, 32>& LoadChains, bool EnsureSingleUser=false);
        void prepopulateLoadChains(llvm::Loop* I, SmallPtrSet<Instruction*, 32>& LoadChains);

        /// data members for local-sinking
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> LocalBlkSet;
        llvm::SmallPtrSet<llvm::Instruction*, 8> LocalInstSet;
        /// data members for undo
        std::vector<llvm::Instruction*> MovedInsts;
        std::vector<llvm::Instruction*> UndoLocas;
        /// dumping
        std::string Log;
        llvm::raw_string_ostream LogStream;

        void dumpToFile(const std::string& Log);

        // memoize all possible stores for every loop that is a candidate for sinking
        typedef llvm::SmallVector<llvm::Instruction*, 32> StoresVec;
        llvm::DenseMap<llvm::Loop*, StoresVec> MemoizedStoresInLoops;
        llvm::SmallPtrSet<llvm::Loop*, 8> BlacklistedLoops;
        StoresVec getAllStoresInLoop(llvm::Loop* L);

        /// checking if sinking is beneficial
        llvm::DenseMap<llvm::BasicBlock*, uint> BBPressures;
        LoopSinkMode needLoopSink(llvm::Loop* L);
        unsigned getMaxRegCountForLoop(llvm::Loop* L);
    };

    void initializeCodeLoopSinkingPass(llvm::PassRegistry&);

}

