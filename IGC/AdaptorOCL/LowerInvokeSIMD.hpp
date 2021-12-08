/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/ValueMap.h>
#include <llvmWrapper/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

#include <string>

namespace IGC
{
    class LowerInvokeSIMD : public llvm::ModulePass, public llvm::InstVisitor<LowerInvokeSIMD>
    {
    public:
        static char ID;

        LowerInvokeSIMD();

        virtual llvm::StringRef getPassName() const override
        {
            return "LowerInvokeSIMD";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnModule(llvm::Module& F) override;
        void visitCallInst(llvm::CallInst& CI);

    private:
        IGCLLVM::IRBuilder<>* m_Builder = nullptr;
        llvm::ValueMap<llvm::Function*, llvm::Function*> m_OldFuncToNewFuncMap;
        bool m_changed = false;

        void detectUniformParams(const llvm::Function* ESIMDFunction, llvm::CallInst& NewCall);
    };
}
