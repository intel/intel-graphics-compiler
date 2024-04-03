/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

namespace IGC
{
class GenerateBlockMemOpsPass : public llvm::FunctionPass
    {
    public:
        static char ID;

        GenerateBlockMemOpsPass();

        virtual llvm::StringRef getPassName() const override {
            return "Generate block memory operations";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<WIAnalysis>();
        }

        virtual bool runOnFunction(llvm::Function &F) override;
    private:
        llvm::Value *checkGep(llvm::GetElementPtrInst *Gep);
        llvm::Value *getLocalId(llvm::Function *F, IGC::ImplicitArg::ArgType Id);
        bool isAddressAligned(llvm::Value *Ptr, const alignment_t &CurrentAlignment, llvm::Type *DataType);
        bool isIndexContinuous(llvm::Value *Addr);
        bool checkVectorizationAlongX(llvm::Function *F);
        bool changeToBlockInst(llvm::Instruction *I);
        bool canOptLoadStore(llvm::Instruction *I);

        WIAnalysis *WI = nullptr;
        IGC::CodeGenContext *CGCtx = nullptr;
        IGC::IGCMD::MetaDataUtils *MdUtils = nullptr;
    };
}