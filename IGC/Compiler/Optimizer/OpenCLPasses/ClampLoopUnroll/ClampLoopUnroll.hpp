/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "llvmWrapper/IR/InstVisitor.h"
#include <llvm/IR/Instruction.h>
#include <llvmWrapper/IR/InstrTypes.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    /// @brief  Cap full unrolls of big loops to bound compile time.
    class ClampLoopUnroll : public llvm::FunctionPass, public llvm::InstVisitor<ClampLoopUnroll>
    {
    public:
        /// @brief  Pass identification.
        static char ID;

        ClampLoopUnroll();

        ClampLoopUnroll(unsigned maxUnrollFactor);

        ~ClampLoopUnroll() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ClampLoopUnroll";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
        }

        void visitTerminatorInst(IGCLLVM::TerminatorInst& I);

    private:
        unsigned m_MaxUnrollFactor;
        bool m_Changed;
    };

} // namespace IGC
