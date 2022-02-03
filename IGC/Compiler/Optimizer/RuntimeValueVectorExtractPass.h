/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

////////////////////////////////////////////////////////////////////////////
// @brief Converts extracts of elements from corresponding RuntimeValue
// vector to RuntimeValue calls returning concrete scalars.
// Only extracts using constant indexes are converted.
// Only 32-bit and 64-bit RuntimeValues are supported at the moment.
//
// Replace:
//   %0 = call <8 x i32> @llvm.genx.GenISA.RuntimeValue.v8i32(i32 4)
//   %scalar  = extractelement <8 x i32> %0, i32 0
//   %scalar1 = extractelement <8 x i32> %0, i32 1
// with:
//   %scalar  = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//   %scalar1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/ADT/StringRef.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class RuntimeValueVectorExtractPass : public llvm::FunctionPass, public llvm::InstVisitor<RuntimeValueVectorExtractPass>
    {
    public:
        RuntimeValueVectorExtractPass();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        virtual llvm::StringRef getPassName() const override
        {
            return "RuntimeValueVectorExtractPass";
        }

        void visitExtractElementInst(llvm::ExtractElementInst& I);

        bool changed;

        static char ID;
    };
}
