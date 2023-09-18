/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class BreakdownIntrinsicPass : public llvm::FunctionPass, public llvm::InstVisitor<BreakdownIntrinsicPass>
    {
    public:

        // Pass identification, replacement for typeid
        static char ID;

        BreakdownIntrinsicPass();

        ~BreakdownIntrinsicPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "BreakdownIntrinsicPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitIntrinsicInst(llvm::IntrinsicInst& I);

    private:
        bool m_changed;
        IGC::IGCMD::MetaDataUtils* m_pMdUtils;
        IGC::ModuleMetaData* modMD;
    };

}
