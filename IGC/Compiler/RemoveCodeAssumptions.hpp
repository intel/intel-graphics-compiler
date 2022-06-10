/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class RemoveCodeAssumptions : public llvm::FunctionPass, public llvm::InstVisitor<RemoveCodeAssumptions>
    {
    public:
        static char ID;

        RemoveCodeAssumptions();
        ~RemoveCodeAssumptions() {}

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "RemoveCodeAssumptions Pass";
        }

        void visitIntrinsicInst(llvm::IntrinsicInst& I);

    private:
        std::vector<llvm::Instruction*> m_instructionsToRemove;
    };
} // namespace IGC
