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
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/VectorShuffleAnalysis.hpp"
#include "Compiler/CISACodeGen/IGCLivenessAnalysis.h"
#include "Compiler/CISACodeGen/TranslationTable.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

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
        bool localSink(BasicBlock* BB);
        void rollbackSinking(BasicBlock* BB);

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
    typedef enum { Unknown=0, MaybeSink, Sink, IntraLoopSink } LoopSinkWorthiness;

    class CodeLoopSinking : public llvm::FunctionPass {
        llvm::DominatorTree* DT = nullptr;
        llvm::LoopInfo* LI = nullptr;
        llvm::AliasAnalysis* AA = nullptr;
        WIAnalysisRunner* WI;
        IGCMD::MetaDataUtils* MDUtils = nullptr;
        IGCLivenessAnalysis* RPE = nullptr;
        IGCFunctionExternalRegPressureAnalysis* FRPE = nullptr;
        CodeGenContext* CTX = nullptr;
        TargetLibraryInfo* TLI = nullptr;

    public:
        static char ID; // Pass identification

        CodeLoopSinking();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();

            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<llvm::AAResultsWrapperPass>();
            AU.addRequired<VectorShuffleAnalysis>();
            AU.addRequired<IGCLivenessAnalysis>();
            AU.addRequired<IGCFunctionExternalRegPressureAnalysis>();
            AU.addRequired<TargetLibraryInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();

            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
            AU.addPreserved<llvm::AAResultsWrapperPass>();
        }
    private:

        typedef enum VerbosityLevel {
            None = 0,
            Low,
            Medium,
            High
        } VerbosityLevel;

        typedef llvm::SmallPtrSet<llvm::Instruction*, 32> InstSet;
        typedef llvm::SmallVector<llvm::Instruction*, 16> InstrVec;

        // Candidate for a sinking - POD structure to describe instructions to be sinked in a loop

        // Instructions - list of instructions to be sinked in reverse order (from last to first)
        //     All instructions must be from the same basic block, not necessarily consecutive
        // TgtBB - target basic block where instructions should be sinked
        // Worthiness - worthiness of the candidate to be sinked
        //     Unknown - candidate is not checked yet
        //     MaybeSink - candidate is checked and can be sinked if it's beneficial
        //         (it depends on whether the other candidates that use it's operands are going to be sinked)
        //     Sink - candidate is checked and should be sinked
        //     IntraLoopSink - candidate is already in the loop, but might be sinked to another basic block
        //         or scheduled within the basic block

        struct Candidate {

            Candidate(const InstrVec& Instructions, BasicBlock* TgtBB, LoopSinkWorthiness Worthiness, llvm::Instruction* UndoPos)
                : Instructions(Instructions), TgtBB(TgtBB), Worthiness(Worthiness), UndoPos(UndoPos) {}

            Candidate(llvm::Instruction* Instruction, BasicBlock* TgtBB, LoopSinkWorthiness Worthiness, llvm::Instruction* UndoPos)
                : Instructions(InstrVec{Instruction}), TgtBB(TgtBB), Worthiness(Worthiness), UndoPos(UndoPos) {}

            Candidate(const Candidate& Other)
                : Instructions(Other.Instructions), TgtBB(Other.TgtBB), Worthiness(Other.Worthiness), UndoPos(Other.UndoPos) {}

            Candidate& operator=(const Candidate&) = delete;

            InstrVec Instructions;

            BasicBlock *TgtBB;
            LoopSinkWorthiness Worthiness = Unknown;
            Instruction *UndoPos = nullptr;

            // iterator of instructions
            typedef llvm::SmallVector<llvm::Instruction*, 16>::iterator iterator;
            iterator begin() { return Instructions.begin(); }
            iterator end() { return Instructions.end(); }

            // const iterator of instructions
            typedef llvm::SmallVector<llvm::Instruction*, 16>::const_iterator const_iterator;
            const_iterator begin() const { return Instructions.begin(); }
            const_iterator end() const { return Instructions.end(); }

            // first instruction - comes last in the BB but first in the list
            Instruction *first() const { return Instructions.front(); }

            size_t size() const { return Instructions.size(); }

            void print(llvm::raw_ostream& OS) const {
                auto worthinessToString = [](LoopSinkWorthiness Worthiness) {
                    switch (Worthiness) {
                    case Unknown: return "Unknown";
                    case MaybeSink: return "MaybeSink";
                    case Sink: return "Sink";
                    case IntraLoopSink: return "IntraLoopSink";
                    default: return "Unknown";
                    }
                };

                OS << "Candidate: Target BB: " << TgtBB->getName() << " Worthiness: " << worthinessToString(Worthiness) << " Instructions: \n";
                for (auto I : Instructions) {
                    OS << *I << " ";
                }
            }
        };

        using CandidateVec = llvm::SmallVector<std::unique_ptr<Candidate>, 64>;
        using CandidatePtrVec = llvm::SmallVector<Candidate *, 64>;
        using CandidatePtrSet = llvm::DenseSet<Candidate *>;
        using InstToCandidateMap = llvm::MapVector<Instruction *, Candidate *>;

        /// sinking
        bool loopSink(llvm::Function& F);
        bool loopSink(llvm::Loop* LoopWithPressure, LoopSinkMode Mode);

        bool localSink(llvm::BasicBlock* BB, InstToCandidateMap& InstToCandidate, bool Aggressive=false);

        /// candidates creation
        bool tryCreateShufflePatternCandidates(
            llvm::BasicBlock* BB,
            llvm::Loop* L,
            InstSet& SkipInstructions,
            CandidateVec& SinkCandidates
        );
        bool tryCreate2dBlockReadGroupSinkingCandidate(
            llvm::Instruction *I,
            llvm::Loop *L,
            InstSet& SkipInstructions,
            CandidateVec& SinkCandidates
        );
        CandidateVec refineLoopSinkCandidates(
            CandidateVec& SinkCandidates,
            InstSet& LoadChains,
            llvm::Loop* L);

        bool isSafeToLoopSinkLoad(llvm::Instruction* I, llvm::Loop* Loop);
        bool isAlwaysSinkInstruction(llvm::Instruction* I);
        bool allUsesAreInLoop(llvm::Instruction* I, llvm::Loop* L);

        BasicBlock* findLowestLoopSinkTarget(llvm::Instruction* I, llvm::Loop* L);

        /// load chain heuristic
        bool isLoadChain(llvm::Instruction* I, InstSet& LoadChains, bool EnsureSingleUser=false);
        void prepopulateLoadChains(llvm::Loop* I, InstSet& LoadChains);

        /// data members for local-sinking
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> LocalBlkSet;
        /// data members for undo
        llvm::SmallPtrSet<llvm::BasicBlock*, 8> UndoBlkSet;
        /// dumping
        std::string Log;
        llvm::raw_string_ostream LogStringStream;
        llvm::raw_ostream *LogStream = nullptr;

        void dumpToFile(const std::string& Log);

        // memoize all possible stores for every loop that is a candidate for sinking
        typedef llvm::SmallVector<llvm::Instruction*, 32> StoresVec;
        llvm::DenseMap<llvm::Loop*, StoresVec> MemoizedStoresInLoops;
        llvm::SmallPtrSet<llvm::Loop*, 8> BlacklistedLoops;
        StoresVec getAllStoresInLoop(llvm::Loop* L);

        /// checking if sinking in a particular loop is beneficial
        llvm::DenseMap<llvm::BasicBlock*, uint> BBPressures;
        bool mayBeLoopSinkCandidate(llvm::Instruction* I, llvm::Loop* L);
        unsigned getMaxRegCountForLoop(llvm::Loop* L);
        unsigned getMaxRegCountForFunction(llvm::Function* F);
        LoopSinkMode needLoopSink(llvm::Loop* L);
    };

    void initializeCodeLoopSinkingPass(llvm::PassRegistry&);

}

