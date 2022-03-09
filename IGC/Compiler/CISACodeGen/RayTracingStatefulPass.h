/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/StringMacros.hpp"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"  // for suppressing LLVM warnings
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"   // for suppressing LLVM warnings

namespace IGC {

class RaytracingStatefulPass : public llvm::FunctionPass
{
public:
    RaytracingStatefulPass() : FunctionPass(ID) {}

    llvm::StringRef getPassName() const override
    {
        return "RaytracingStatefulPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(llvm::Function& function) override;

    static char ID;
};

inline llvm::FunctionPass* createRaytracingStatefulPass()
{
    return new RaytracingStatefulPass();
}

}  // namespace IGC

