/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

////////////////////////////////////////////////////////////////////////////
// @brief Legalizes RuntimeValue calls for push analysis. RuntimeValue calls
// returning single scalars are converted to extracts of elements
// from corresponding RuntimeValue vector, but only in case such a vector exists.
// Only 32-bit and 64-bit RuntimeValues are supported at the moment.
//
// Replace:
//   %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//  %14 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
// with:
//   %4 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//   %2 = extractelement <3 x i32> %1, i32 0
//  %15 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//  %16 = extractelement <3 x i32> %15, i32 1

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Module.h"
#include "llvm/ADT/StringRef.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class RuntimeValueLegalizationPass : public llvm::ModulePass
    {
    public:
        RuntimeValueLegalizationPass();

        virtual bool runOnModule(llvm::Module& module) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        virtual llvm::StringRef getPassName() const override
        {
            return "RuntimeValueLegalizationPass";
        }

        static char ID;
    };
}
