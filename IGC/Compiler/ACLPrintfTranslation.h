/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/Optimizer/OCLBIUtils.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC
{
    class ACLPrintfTranslation : public llvm::ModulePass, public llvm::InstVisitor < ACLPrintfTranslation >
    {
    public:
        static char ID;

        ACLPrintfTranslation();

        ~ACLPrintfTranslation() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnModule(llvm::Module& M) override;
        void visitCallInst(llvm::CallInst& CI);

        virtual llvm::StringRef getPassName() const override
        {
            return "Translate ACL Printf";
        }

    };

} // namespace IGC
