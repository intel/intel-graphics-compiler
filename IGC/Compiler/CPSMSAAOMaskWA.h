/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/ADT/SmallSet.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CPSMSAAOMaskWA : public llvm::ModulePass, public llvm::InstVisitor<CPSMSAAOMaskWA>
    {
    public:
        static char ID;

        CPSMSAAOMaskWA();

        ~CPSMSAAOMaskWA() {}

        void visitCallInst(llvm::CallInst& I);

        virtual bool runOnModule(llvm::Module& M) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "CPSMSAAOMaskWorkaround Pass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

    private:
        llvm::Module* m_pModule;
        std::vector<llvm::GenIntrinsicInst*> m_outputs;
        llvm::GenIntrinsicInst* m_oMaskInst;
    };
} // namespace IGC
