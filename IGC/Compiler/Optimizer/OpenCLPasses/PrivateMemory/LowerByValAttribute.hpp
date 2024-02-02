/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class LowerByValAttribute : public llvm::FunctionPass, public llvm::InstVisitor<LowerByValAttribute>
    {
    public:
        static char ID;

        LowerByValAttribute();

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitCallInst(llvm::CallInst& CI);

    private:
        bool m_changed = false;
    };

} // namespace IGC
