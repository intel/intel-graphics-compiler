/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include <unordered_map>

namespace IGC
{
    class PromoteStatelessToBindless : public llvm::FunctionPass, public llvm::InstVisitor<PromoteStatelessToBindless>
    {
    public:
        static char ID;

        PromoteStatelessToBindless();

        virtual llvm::StringRef getPassName() const override
        {
            return "PromoteStatelessToBindless";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        bool runOnFunction(llvm::Function& F) override;
        void visitInstruction(llvm::Instruction& I);

    private:
        void GetAccessInstToSrcPointerMap(llvm::Instruction* inst, llvm::Value* resourcePtr);
        void PromoteStatelessToBindlessBuffers(llvm::Function& F) const;
        void CheckPrintfBuffer(llvm::Function& F);

        std::unordered_map<llvm::Value*, llvm::Value*> m_AccessToSrcPtrMap;
        std::unordered_map<llvm::Value*, llvm::Value*> m_AddressUsedSrcPtrMap;
        llvm::Value* m_PrintfBuffer;
    };

} // namespace IGC
