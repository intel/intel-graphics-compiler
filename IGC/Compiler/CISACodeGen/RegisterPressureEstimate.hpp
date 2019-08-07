//===-- RegisterPressureEstimate - Estimate Register Pressure -----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LRCENSE.TXT for details.
//
//  Copyright  (C) 2014 Intel Corporation. All rights reserved.
//
// The information and source code contained herein is the exclusive
// property of Intel Corporation and may not be disclosed, examined
// or reproduced in whole or in part without explicit written authorization
// from the company.
//
//===----------------------------------------------------------------------===//
//
// Estimate the register pressure at a program point.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include <llvm/IR/InstVisitor.h>
#include "llvm/Analysis/LoopInfo.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"

namespace IGC
{
    class RegisterPressureEstimate : public llvm::FunctionPass
    {
    public:
        static char ID;
        RegisterPressureEstimate()
            : FunctionPass(ID), m_pFunc(nullptr), LI(nullptr), m_available(false)
        {
            initializeRegisterPressureEstimatePass(*llvm::PassRegistry::getPassRegistry());
        }

        /// @brief  Provides name of pass
        virtual llvm::StringRef getPassName() const override
        {
            return "RegisterPressureEstimate";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
        bool runOnFunction(llvm::Function& F) override;

        bool doFinalization(llvm::Module& F) override
        {
            for (auto Item : m_pLiveRangePool)
                delete Item;
            m_pLiveRangePool.clear();
            return false;
        }
        /// \brief Describe a value is live at Begin and dead right after End. 
        struct Segment
        {
            unsigned Begin;
            unsigned End;
            Segment() {}
            Segment(unsigned Begin, unsigned End) : Begin(Begin), End(End) {}
            bool operator<(const Segment& Other) const
            {
                if (Begin != Other.Begin)
                {
                    return Begin < Other.Begin;
                }
                return End < Other.End;
            }
        };

        /// \brief A live range consists of a list of live segments.
        struct LiveRange
        {
            /// Empty live range.
            LiveRange() {}

            LiveRange(const LiveRange&) = delete;

            /// \brief Shorten a segment.
            void setBegin(unsigned B);

            /// \brief Append a new segment.
            void addSegment(unsigned B, unsigned E)
            {
                if (B < E)
                {
                    Segments.push_back(Segment(B, E));
                }
            }

            /// \brief Sort segments and merge them. 
            void sortAndMerge();

            /// \brief Check whether N is contained in this live range.
            bool contains(unsigned N) const
            {
                // Segments are sorted.
                for (auto& Seg : Segments)
                {
                    if (N < Seg.Begin)
                    {
                        return false;
                    }
                    if (N < Seg.End)
                    {
                        return true;
                    }
                }
                // N is out of range.
                return false;
            }

            void dump() const;
            void print(llvm::raw_ostream& OS) const;
            llvm::SmallVector<Segment, 8> Segments;
        };

        LiveRange* getOrCreateLiveRange(llvm::Value* V)
        {
            auto Iter = m_pLiveRanges.find(V);
            if (Iter != m_pLiveRanges.end())
            {
                return Iter->second;
            }

            auto Result = m_pLiveRanges.insert(std::make_pair(V, createLiveRange()));
            return Result.first->second;
        }

        LiveRange* getLiveRangeOrNull(llvm::Value* V)
        {
            auto Iter = m_pLiveRanges.find(V);
            if (Iter != m_pLiveRanges.end())
            {
                return Iter->second;
            }
            return nullptr;
        }

        void createLiveRange(llvm::Value* V)
        {
            assert(!m_pLiveRanges.count(V));
            m_pLiveRanges[V] = createLiveRange();
        }

        /// \brief Cleanup live ranges.
        void mergeLiveRanges()
        {
            for (auto& Item : m_pLiveRanges)
            {
                Item.second->sortAndMerge();
            }
        }

        /// \brief Assign a number to each instruction in a function.
        void assignNumbers();

        /// \brief Print instruction list with numbering.
        void printNumbering(llvm::raw_ostream& OS);
        void dumpNumbering();

        /// \brief Used to fetch number on instructions and BB
        unsigned getAssignedNumberForInst(llvm::Instruction*);
        unsigned getMaxAssignedNumberForFunction();
        unsigned getMaxAssignedNumberForBB(llvm::BasicBlock*);
        unsigned getMinAssignedNumberForBB(llvm::BasicBlock* pBB);

        /// \brief Print live ranges.
        void printLiveRanges(llvm::raw_ostream& OS);
        void dumpLiveRanges();

        /// \brief Scan a function and build live ranges for all values of interest. A
        /// live range consists of non-overlapping live intervals [i, j) where i is
        /// the label this value starts to live and j is the label where it ends
        /// living. This value may still be used at j but it does not interfere with
        /// the value defined at j. Thus intervals are open on the right-hand side.
        /// The return value of true indicates that live ranges are available for use.
        bool buildLiveIntervals(bool RemoveLR = false);

        /// \brief Perform a function-wide liveness analysis.
        void analyzeLifetime();

        /// \brief Return the register pressure for the whole function.
        unsigned getRegisterPressure() const;

        /// \brief Return the register pressure for a basic block.
        unsigned getRegisterPressure(llvm::BasicBlock* BB) const;

        void printRegisterPressureInfo(bool Detailed = false,
            const char* msg = "") const;

        bool isAvailable() const { return m_available; }

        void clear(bool RemoveLR)
        {
            for (auto& Item : m_pLiveRanges)
                Item.second->Segments.clear();
            if (RemoveLR)
                m_pLiveRanges.clear();
            for (auto Item : m_pLiveRangePool)
                delete Item;
            m_pLiveRangePool.clear();
        }

        unsigned getRegisterPressureForInstructionFromRPMap(unsigned number) const;

        void buildRPMapPerInstruction();

    private:
        /// \brief Return the register pressure at location specified by Inst.
        unsigned getRegisterPressure(llvm::Instruction* Inst) const;

        LiveRange* createLiveRange()
        {
            LiveRange* LR = new LiveRange();
            m_pLiveRangePool.push_back(LR);
            return LR;
        }

    private:
        /// The function being analyzed.
        llvm::Function* m_pFunc;

        /// The loop info object.
        llvm::LoopInfo* LI;

        /// Each instruction gets an ID.
        llvm::DenseMap<llvm::Value*, unsigned> m_pNumbers;

        /// The value to live range map.
        std::map<llvm::Value*, LiveRange*> m_pLiveRanges;

        /// To check if live range info is available
        bool m_available;

        std::vector<LiveRange*> m_pLiveRangePool;

        llvm::DenseMap<unsigned, unsigned> m_pRegisterPressureByInstruction;
    };
} // namespace IGC
