/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class UnreachableHandling : public llvm::FunctionPass, public llvm::InstVisitor<UnreachableHandling>
    {
    public:
        static char ID;

        UnreachableHandling();
        ~UnreachableHandling() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "UnreachableHandling";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
        }

        void visitUnreachableInst(llvm::UnreachableInst& I);

    private:
        bool m_changed = false;
        std::vector<llvm::UnreachableInst*> m_instsToReplace;

        void replaceUnreachable(llvm::UnreachableInst* I);
    };

} // namespace IGC
