/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class FixFastMathFlags : public llvm::FunctionPass, public llvm::InstVisitor<FixFastMathFlags>
    {
    public:
        static char ID;

        FixFastMathFlags();

        virtual llvm::StringRef getPassName() const override
        {
            return "Fix Fast Math Flags";
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitFCmpInst(llvm::FCmpInst& FC);

    private:
        bool m_changed = false;
    };

} // namespace IGC
