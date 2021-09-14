/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

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
    class JointMatrixFuncsResolutionPass final
        : public llvm::FunctionPass
        , public llvm::InstVisitor<JointMatrixFuncsResolutionPass>
    {
    public:
        static char ID;

        JointMatrixFuncsResolutionPass(OpenCLProgramContext *Context);
        ~JointMatrixFuncsResolutionPass() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "JointMatrixFuncsResolutionPass";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<IGC::MetaDataUtilsWrapper>();
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        void visitCallInst(llvm::CallInst& CI);

    private:
        llvm::Instruction *ResolveLoad(llvm::CallInst *CI);
        llvm::Instruction *ResolveStore(llvm::CallInst *CI);
        llvm::Instruction *ResolveMad(llvm::CallInst *CI);
        llvm::Type *ResolveType(const llvm::Type *opaqueType, uint32_t elementTypeFlags, unsigned rows, unsigned *outLayout);
        llvm::Value *Resolve(llvm::Value *value);

        std::string GetLoadStoreMatrixFuncName
            (bool load, unsigned opLayout, unsigned matrixLayout, unsigned elemBitWidth, unsigned rows, unsigned cols);
        std::string getMADMatrixFuncName
            (uint32_t aTypeFlags, uint32_t bTypeFlags, uint32_t cTypeFlags, unsigned M, unsigned N);

        llvm::ValueMap<llvm::Value *, llvm::Value *> ResolvedValues;
        llvm::SmallPtrSet<llvm::Instruction *, 8> InstsToErase;

        ModuleMetaData* MMD;
        OpenCLProgramContext* Context;
        bool Changed;
    };
};
