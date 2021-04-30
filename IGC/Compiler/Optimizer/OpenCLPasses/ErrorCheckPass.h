/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class ErrorCheck : public llvm::FunctionPass, public llvm::InstVisitor<ErrorCheck>
    {
    public:
        static char ID;

        ErrorCheck();

        virtual llvm::StringRef getPassName() const override
        {
            return "Error Check";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        void visitInstruction(llvm::Instruction& I);

        void visitCallInst(llvm::CallInst& CI);

    private:
        bool m_hasError = false;

        void checkArgsSize(llvm::Function& F);
    };

} // namespace IGC

