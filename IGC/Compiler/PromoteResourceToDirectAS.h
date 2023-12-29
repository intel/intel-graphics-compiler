/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/CISACodeGen.h"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include <unordered_map>

namespace IGC
{
    class PromoteResourceToDirectAS : public llvm::FunctionPass, public llvm::InstVisitor<PromoteResourceToDirectAS>
    {
    public:
        static char ID;

        PromoteResourceToDirectAS();

        virtual llvm::StringRef getPassName() const override
        {
            return "PromoteResourceToDirectAS";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        bool            runOnFunction(llvm::Function& F) override;
        void            visitInstruction(llvm::Instruction& I);

    private:
        void PromoteSamplerTextureToDirectAS(llvm::GenIntrinsicInst*& pIntr, llvm::Value* resourcePtr);
        void PromoteBufferToDirectAS(llvm::Instruction* inst, llvm::Value* resourcePtr);
        llvm::Value* getOffsetValue(llvm::Value* srcPtr, int& bufferoffset);

        CodeGenContext* m_pCodeGenContext = nullptr;
        IGCMD::MetaDataUtils* m_pMdUtils = nullptr;
        std::unordered_map<llvm::Value*, llvm::Value*> m_SrcPtrToBufferOffsetMap;
    };

} // namespace IGC
