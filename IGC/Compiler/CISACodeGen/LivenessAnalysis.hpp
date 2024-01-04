/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/CISACodeGen.h"
#include "Compiler/CISACodeGen/LiveVars.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SparseBitVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include <llvm/Support/Allocator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

namespace IGC
{
    typedef llvm::SparseBitVector<>                         SBitVector;
    typedef llvm::SmallVector<llvm::Value*, 4>              ValueVec;
    typedef llvm::DenseMap<llvm::BasicBlock*, SBitVector>   BBLiveInMap;
    typedef llvm::DenseMap<llvm::Value*, ValueVec>          ValueToValueVecMap;
    typedef llvm::DenseMap<llvm::Value*, int>               ValueToIntMap;
    typedef llvm::SmallVector<llvm::Value*, 32>             IntToValueVector;

    //  LivenessAnalysis compute liveness information based on LiveVars.
    //  It has three kinds of information: IN set, defInst, killInsts
    //     IN:  live-in set, one for each BB (BBLiveInMap)
    //     defInst: def instruction. Since it is a SSA, Value itself
    //              denotes that.
    //     killInsts: Given an inst, killInsts has all values that have
    //              their last uses at this inst (ValueToValueSetMap).
    class LivenessAnalysis : public llvm::FunctionPass {
    public:

        static char ID; // Pass identification, replacement for typeid

        LivenessAnalysis() :
            llvm::FunctionPass(ID),
            m_LV(nullptr),
            m_F(nullptr),
            m_WIA(nullptr)
        {
            initializeLivenessAnalysisPass(*llvm::PassRegistry::getPassRegistry());
        }

        ~LivenessAnalysis();
        LivenessAnalysis(const LivenessAnalysis&) = delete;
        LivenessAnalysis& operator=(const LivenessAnalysis&) = delete;

        // Liveness is computed on demand, by explicitly calling calculate().
        // runOnFunction does not calculate it!
        //
        // This gives the liveness users' the full control, so that the users
        // will only calculate liveness when it is needed. Doing so will save
        // compiling time.
        //
        // Note that runOnFunction() will set up ValueIds map as PreRAScheduler
        // uses it directly, even though Liveness is not computed.
        bool runOnFunction(llvm::Function& F) override;

        // Entry to compute Liveness
        void calculate(llvm::Function* F);

        llvm::StringRef getPassName() const override { return "LivenessAnalysis"; }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            //AU.addRequired<LiveVarsAnalysis>();
            AU.setPreservesAll();
        }

        // Return the distance of instruction within BB (start from 0).
        // (See LiveVars for the detail of distance)
        uint32_t getDistance(llvm::Instruction* I)
        {
            return m_LV->getDistance(I);
        }

        LiveVars* getLiveVars() { return m_LV; }

        llvm::Value* getValueFromBitId(int BitId)
        {
            llvm::Value* V = nullptr;
            if (BitId < (int)IdValues.size())
            {
                V = IdValues[BitId];
            }
            IGC_ASSERT_MESSAGE(nullptr != V, "Invalid bit id found!");
            return V;
        }

        // Return true if instruction I has the last use of V.
        bool isInstLastUseOfValue(llvm::Value* V, llvm::Instruction* I);
        // For A and B that are in the same BB, check if A appears before B.
        bool isBefore(llvm::Instruction* A, llvm::Instruction* B);

        // Return true if V is uniform
        bool isUniform(llvm::Value* V) const
        {
            return m_WIA && m_WIA->isUniform(V);
        }

        bool isCandidateValue(llvm::Value* V)
        {
            return ValueIds.count(V) > 0;
        }

        uint32_t getNumValues() const { return IdValues.size(); }

        // release all memory.
        void clear();

        virtual void releaseMemory() override {
            clear();
        }

    private:

        LiveVars* m_LV;
        llvm::Function* m_F;
        WIAnalysis* m_WIA;  // Optional

        void initValueIds();
        void setLiveIn(llvm::BasicBlock* BB, llvm::Value* V);
        void setLiveIn(llvm::BasicBlock* BB, int ValueID);
        void setKillInsts(llvm::Value* V, llvm::Instruction* kill);

    public:
        // Value --> its ID  & ID --> Value
        ValueToIntMap    ValueIds;
        IntToValueVector IdValues;

        // IN set, one for each BB
        BBLiveInMap  BBLiveIns;

        // Instruction (first, as value) and all values whose last uses are
        // at this instruction.
        ValueToValueVecMap KillInsts;

        /// print - Convert to human readable form
        void print(llvm::raw_ostream& OS);

        /// print_livein : print live in set for a BB
        void print_livein(llvm::raw_ostream& OS, llvm::BasicBlock* BB);

#if defined( _DEBUG )
        /// dump - Dump the liveness info to dbgs(), used in debugger.
        void dump();
        void dump_livein();
        void dump_livein(llvm::BasicBlock* BB);
#endif

        // May be placed somewhere else
        static std::string getllvmValueName(llvm::Value* V);
    };
}
