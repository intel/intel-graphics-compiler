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

#include "AllocationLivenessAnalyzer.h"

namespace llvm {
    class Function;
} // namespace llvm

namespace IGC
{
    class MergeAllocas : public AllocationLivenessAnalyzer
    {
    public:
        struct AllocaInfo {
            llvm::SmallVector<AllocaInfo*> nonOverlapingAllocas;
            llvm::AllocaInst* allocaI;
            AllocationLivenessAnalyzer::LivenessData* livenessData;
            unsigned int addressSpace;
            std::size_t allocationSize;
            std::size_t remainingSize;
            std::size_t alignment;
            // start offset of this alloca in top level alloca (if any)
            std::size_t offset;
            bool isUniform;
        };

        MergeAllocas(char& pid) : AllocationLivenessAnalyzer(pid) {}

        bool runOnFunction(llvm::Function& F) override;
        virtual bool skipInstruction(llvm::Function& F, AllocationLivenessAnalyzer::LivenessData& LD) = 0;
        void getAdditionalAnalysisUsage(llvm::AnalysisUsage& AU) const override {};

    private:
        std::vector<AllocaInfo> AllAllocasInfos;
    };
} // namespace IGC
