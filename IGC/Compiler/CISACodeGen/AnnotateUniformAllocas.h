/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/ADT/SmallVector.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    llvm::FunctionPass* createAnnotateUniformAllocasPass();

    class AnnotateUniformAllocas : public llvm::FunctionPass, public llvm::InstVisitor<AnnotateUniformAllocas>
    {
    public:
        static char ID;

        AnnotateUniformAllocas();

        virtual llvm::StringRef getPassName() const override
        {
            return "Annotate Uniform Allocas Pass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<WIAnalysis>();
            AU.setPreservesCFG();
            // expect to run WIA again after this pass puts annotations on uniform alloca
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitAllocaInst(llvm::AllocaInst& I);
        void visitCallInst(llvm::CallInst& I);

    private:
        WIAnalysis* WI = nullptr;
        bool m_changed = false;
        llvm::SmallVector<llvm::Instruction*, 4> AssumeToErase;
    };
} // namespace IGC
