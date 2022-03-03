/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

////////////////////////////////////////////////////////////////////////////
// @brief Legalizes RuntimeValue calls for push analysis.
//
// 1) RuntimeValue vector must be GRF aligned if it's size is larger than or equal to one GRF.
//    RuntimeValue vector must fit in one GRF if its size is less than one GRF.
//    Replace:
//      %15 = call <6 x i32> @llvm.genx.GenISA.RuntimeValue.v6i32(i32 4)
//      %17 = extractelement <6 x i32> %15, i32 %0
//    with:
//      %15 = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
//      %16 = add i32 %0, 4
//      %17 = extractelement <10 x i32> %15, i32 %16
//
// 2) RuntimeValue vectors can not overlap:
//    Replace:
//      %15 = call <10 x i32> @llvm.genx.GenISA.RuntimeValue.v10i32(i32 0)
//      %17 = extractelement <10 x i32> %15, i32 %0
//      %25 = call <12 x i32> @llvm.genx.GenISA.RuntimeValue.v12i32(i32 8)
//      %27 = extractelement <12 x i32> % 25, i32 %0
//    with:
//      %15 = call <20 x i32> @llvm.genx.GenISA.RuntimeValue.v20i32(i32 0)
//      %17 = extractelement <20 x i32> %15, i32 %0
//      %25 = call <20 x i32> @llvm.genx.GenISA.RuntimeValue.v20i32(i32 0)
//      %26 = add i32 %0, 8
//      %27 = extractelement <20 x i32> %25, i32 %26
//
// 3) RuntimeValue calls returning single scalars are converted to extracts of elements
//    from corresponding RuntimeValue vector.
//    Replace:
//       %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %3 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//      %14 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
//    with:
//       %4 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %1 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//       %2 = extractelement <3 x i32> %1, i32 0
//      %15 = call <3 x i32> @llvm.genx.GenISA.RuntimeValue.v3i32(i32 4)
//      %16 = extractelement <3 x i32> %15, i32 1
//
// Only RuntimeValue vectors of 32-bit elements are supported at the moment.

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
