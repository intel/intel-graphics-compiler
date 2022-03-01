/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ResolvePointersComparison : public llvm::FunctionPass, public llvm::InstVisitor<ResolvePointersComparison>
    {
    public:
        static char ID;

        ResolvePointersComparison();

        virtual llvm::StringRef getPassName() const override
        {
            return "ResolvePointerComparison";
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitICmpInst(llvm::ICmpInst& FC);

    private:
        bool m_changed = false;
    };

} // namespace IGC
