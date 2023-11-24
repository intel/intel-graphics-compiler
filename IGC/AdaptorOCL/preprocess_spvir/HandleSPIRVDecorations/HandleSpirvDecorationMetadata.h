/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

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
#include "Compiler/CodeGenPublic.h"

#include <string>

namespace IGC
{
    class HandleSpirvDecorationMetadata : public llvm::ModulePass, public llvm::InstVisitor<HandleSpirvDecorationMetadata>
    {
    public:
        static char ID;

        HandleSpirvDecorationMetadata();
        ~HandleSpirvDecorationMetadata() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "HandleSpirvDecorationMetadata";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnModule(llvm::Module& F) override;

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);

    private:
        llvm::Module* m_Module = nullptr;
        CodeGenContext* m_pCtx = nullptr;
        ModuleMetaData* m_Metadata = nullptr;
        bool m_changed = false;

        void handleInstructionsDecorations();
        void handleGlobalVariablesDecorations();

        void handleHostAccessIntel(llvm::GlobalVariable& globalVariable, llvm::MDNode* node);
        template<typename T>
        void handleCacheControlINTEL(llvm::Instruction& I, llvm::SmallPtrSetImpl<llvm::MDNode*>& MDNodes);
        llvm::DenseMap<uint64_t, llvm::SmallPtrSet<llvm::MDNode*, 4>> parseSPIRVDecorationsFromMD(llvm::Value* V);
    };
}
