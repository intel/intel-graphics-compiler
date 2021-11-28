/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/LLVMContext.h>
#include "common/LLVMWarningsPop.hpp"

#include <unordered_set>

namespace IGC
{
    class InsertGenericPtrArithmeticMetadata : public llvm::FunctionPass, public llvm::InstVisitor<InsertGenericPtrArithmeticMetadata>
    {
    public:
        static char ID;

        InsertGenericPtrArithmeticMetadata();

        virtual llvm::StringRef getPassName() const override
        {
            return "InsertGenericPtrArithmeticMetadata";
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        bool hasOnlyArithmeticUses(llvm::Instruction* I);
        void visitAddrSpaceCast(llvm::AddrSpaceCastInst& I);

    private:
        llvm::LLVMContext* m_context = nullptr;
        bool m_changed = false;
        std::unordered_set<llvm::PHINode*> m_visitedPHIs;
    };

} // namespace IGC
