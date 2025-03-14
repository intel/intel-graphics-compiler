/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SetVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
    class BasicBlock;
    class DominatorTree;
    class Function;
    class Instruction;
    class LoopInfo;
}

namespace IGC
{
    class MergeAllocas : public llvm::FunctionPass
    {
    public:
        MergeAllocas();

        bool runOnFunction(llvm::Function& F) override;
        llvm::StringRef getPassName() const override
        {
            return "MergeAllocas";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        static char ID;
    };

    // for now keep this here, in the future this should be generalized and widely available everywhere in IGC
    class AllocationBasedLivenessAnalysis : public llvm::FunctionPass
    {
    public:
        AllocationBasedLivenessAnalysis();
        ~AllocationBasedLivenessAnalysis()
        {
            clearLivenessInfo();
        }

        AllocationBasedLivenessAnalysis (const AllocationBasedLivenessAnalysis &) = delete;
        AllocationBasedLivenessAnalysis & operator=(const AllocationBasedLivenessAnalysis &) = delete;

        bool runOnFunction(llvm::Function& F) override;
        llvm::StringRef getPassName() const override
        {
            return "AllocationBasedLivenessAnalysis";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
        auto getLivenessInfo() { return m_LivenessInfo; }

        static char ID;

        struct LivenessData {
            llvm::Instruction* lifetimeStart = nullptr;
            llvm::SmallVector<llvm::Instruction*> lifetimeEnds;

            llvm::DenseSet<llvm::BasicBlock*> bbIn;
            llvm::DenseSet<llvm::BasicBlock*> bbOut;

            LivenessData(
                llvm::Instruction* allocationInstruction,
                const llvm::SetVector<llvm::Instruction*>& usersOfAllocation,
                const llvm::LoopInfo& LI,
                const llvm::DominatorTree& DT,
                llvm::BasicBlock* userDominatorBlock = nullptr,
                bool isLifetimeInfinite = false
            );

            bool OverlapsWith(const LivenessData& LD) const;
        };

    private:

        llvm::SmallVector<std::pair<llvm::Instruction*, LivenessData*>> m_LivenessInfo;

        LivenessData* ProcessInstruction(llvm::Instruction* I);

        void clearLivenessInfo()
        {
            while (!m_LivenessInfo.empty())
            {
                auto [_, LD] = m_LivenessInfo.pop_back_val();
                delete LD;
            }
        }
    };
} // namespace IGC
