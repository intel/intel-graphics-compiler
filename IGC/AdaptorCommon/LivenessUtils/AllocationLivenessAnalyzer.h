/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/SetVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
    class BasicBlock;
    class DominatorTree;
    class Instruction;
    class LoopInfo;
} // namespace llvm

namespace IGC
{
    class AllocationLivenessAnalyzer : public llvm::FunctionPass
    {
    public:
        struct LivenessData {
            llvm::Instruction* lifetimeStart = nullptr;
            llvm::SmallVector<llvm::Instruction*> lifetimeEndInstructions;

            struct Edge
            {
                llvm::BasicBlock* from;
                llvm::BasicBlock* to;
            };

            llvm::SmallVector<Edge> lifetimeEndEdges;

            llvm::DenseSet<llvm::BasicBlock*> bbIn;
            llvm::DenseSet<llvm::BasicBlock*> bbOut;

            LivenessData(
                llvm::Instruction* allocationInstruction,
                llvm::SetVector<llvm::Instruction*>&& usersOfAllocation,
                const llvm::LoopInfo& LI,
                const llvm::DominatorTree& DT,
                llvm::BasicBlock* userDominatorBlock = nullptr,
                llvm::SetVector<llvm::Instruction*>&& lifetimeLeakingUsers = {}
            );

            bool OverlapsWith(const LivenessData& LD) const;
            bool ContainsInstruction(const llvm::Instruction& I) const;
        };

        AllocationLivenessAnalyzer(char& pid) : llvm::FunctionPass(pid) {}

    protected:
        LivenessData ProcessInstruction(
            llvm::Instruction* I,
            llvm::DominatorTree& DT,
            llvm::LoopInfo& LI
        );

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;
        virtual void getAdditionalAnalysisUsage(llvm::AnalysisUsage& AU) const = 0;
    };

    namespace Provenance
    {
        bool tryFindPointerOrigin(llvm::Value* ptr, llvm::SmallVectorImpl<llvm::Instruction*>& origins);
    }

} // namespace IGC
