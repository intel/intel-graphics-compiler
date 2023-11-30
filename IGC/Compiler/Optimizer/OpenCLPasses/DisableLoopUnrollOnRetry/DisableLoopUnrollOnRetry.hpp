/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/Transforms/Utils/UnrollLoop.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC
{
    /// @brief  Disables loop unroll for each loop by setting llvm.loop.unroll.disable
    class DisableLoopUnrollOnRetry : public LoopPass
    {
    public:
        static char ID;

        DisableLoopUnrollOnRetry();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const
        {
            AU.setPreservesCFG();
        }

        virtual StringRef getPassName() const
        {
            return "DisableLoopUnrollOnRetry";
        }

        virtual bool runOnLoop(Loop* L, LPPassManager& LPM);
    };

} // namespace IGC
