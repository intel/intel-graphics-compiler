/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instruction.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"

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

            LivenessData(llvm::Instruction* allocationInstruction, llvm::SetVector<llvm::Instruction*> usersOfAllocation, llvm::BasicBlock* userDominatorBlock = nullptr);

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
