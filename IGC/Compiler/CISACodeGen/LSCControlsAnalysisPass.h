/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "IGC/common/StringMacros.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class LSCControlsAnalysisPass : public llvm::FunctionPass,
                                    public llvm::InstVisitor<LSCControlsAnalysisPass>
    {
    public:
        LSCControlsAnalysisPass() : FunctionPass(ID) {}
        bool runOnFunction(llvm::Function& M) override;

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }

        static char ID;

        void visitStoreInst(llvm::StoreInst& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitCallInst(llvm::CallInst& CI);

        llvm::StringRef getPassName() const override { return "LSCControlsAnalysis"; }
    private:
        CodeGenContext* m_CGCtx = nullptr;
        bool Changed = false;
    };

    void initializeLSCControlsAnalysisPassPass(llvm::PassRegistry&);
    llvm::FunctionPass* CreateLSCControlsAnalysisPass();
}//namespace IGC
