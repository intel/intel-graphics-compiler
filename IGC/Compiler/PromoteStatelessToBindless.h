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

#include <set>
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

        // Pair of bindless resource access instructions.
        // The first is the actual instruction accessing the bindless buffer (load/store/etc)
        // The second is the instruction accessing the address of the bindless buffer, which may or may not
        // be identical to the first. Needed to convert to null address.
        typedef std::pair<llvm::Value*, llvm::Value*> BindlessAccessInsts;
        // Map of the srcPtr (kernel arg resource) to a vector of instructions accessing it
        llvm::DenseMap<llvm::Value*, llvm::SmallVector<BindlessAccessInsts, 8>> m_SrcPtrToAccessMap;
        // Tracks the set of resources that must have at least one stateless access
        std::set<llvm::Value*> m_SrcPtrNeedStatelessAccess;

        llvm::Value* m_PrintfBuffer;
    };

} // namespace IGC
