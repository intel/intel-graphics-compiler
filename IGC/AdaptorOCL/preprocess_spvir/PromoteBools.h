/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/Module.h"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"

#include <string>

namespace IGC
{
    class PromoteBools : public llvm::ModulePass, public llvm::InstVisitor<PromoteBools>
    {
    public:
        static char ID;

        PromoteBools();
        ~PromoteBools() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "PromoteBools";
        }

        virtual bool runOnModule(llvm::Module& module) override;

        void visitLoadInst(llvm::LoadInst& load);
        void visitStoreInst(llvm::StoreInst& store);
        void visitCallInst(llvm::CallInst& call);

    private:
        bool changed;
        llvm::Type* int1type;
        llvm::Type* int8type;
        llvm::DenseMap<llvm::Value*, llvm::Value*> promotedValuesCache;

        llvm::Value* getOrCreatePromotedValue(llvm::Value* value);

        bool functionNeedsPromotion(llvm::Function* function);
        llvm::Value* createZextIfNeeded(llvm::Value* argument, llvm::Instruction* insertBefore);

        void promoteFunctions(llvm::Module& module);
        void cleanUp(llvm::Module& module);
    };
}
