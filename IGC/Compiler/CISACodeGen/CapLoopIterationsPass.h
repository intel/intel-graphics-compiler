/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// The purpose of this pass is to add a limit for each loop,
// so no infinite loops occur.
//===----------------------------------------------------------------------===//

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC
{
    class CapLoopIterations : public FunctionPass
    {
    public:
        CapLoopIterations();
        CapLoopIterations(uint32_t iterationLimit);

        virtual bool runOnFunction(Function& F) override;

        llvm::StringRef getPassName() const override
        {
            return "CapLoopIterationsPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        static char ID;

    private:
        uint32_t m_iterationLimit = UINT_MAX;
    };

}
