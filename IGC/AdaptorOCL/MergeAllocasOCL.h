/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "../AdaptorCommon/LivenessUtils/MergeAllocas.h"
#include "llvm/PassRegistry.h"

namespace llvm
{
    class PassRegistry;
}

namespace IGC
{
    class MergeAllocasOCL : public MergeAllocas
    {
        bool skipInstruction(llvm::Function& F, AllocationLivenessAnalyzer::LivenessData& LD) override;

    public:
        static char ID;

        MergeAllocasOCL();

        llvm::StringRef getPassName() const override
        {
            return "MergeAllocasOCL";
        }
    };

    llvm::Pass* createMergeAllocasOCL();
}

void initializeMergeAllocasOCLPass(llvm::PassRegistry&);
