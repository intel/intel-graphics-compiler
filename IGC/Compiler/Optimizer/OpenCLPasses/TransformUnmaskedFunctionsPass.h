/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

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
    // Checks if function can be unmased and transforms the attributes of
    // function definition and each call site
    class TransformUnmaskedFunctionsPass final : public llvm::FunctionPass
    {
    public:
        static char ID;

        TransformUnmaskedFunctionsPass();
        ~TransformUnmaskedFunctionsPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "TransformUnmaskedFunctionsPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

    private:
        ModuleMetaData* MMD;
    };

    // Inlines functions marked as unmasked correclty modifying all related metadata.
    class InlineUnmaskedFunctionsPass final : public llvm::ModulePass
    {
    public:
        static char ID;

        InlineUnmaskedFunctionsPass();
        ~InlineUnmaskedFunctionsPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "InlineUnmaskedFunctionsPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnModule(llvm::Module& M) override;

    private:
        ModuleMetaData* MMD;
    };
};
