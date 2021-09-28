/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

#include <string>

namespace IGC
{
    class PreprocessSPVIR : public llvm::ModulePass, public llvm::InstVisitor<PreprocessSPVIR>
    {
    public:
        static char ID;

        PreprocessSPVIR();
        ~PreprocessSPVIR() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "PreprocessSPVIR";
        }

        virtual bool runOnModule(llvm::Module& F) override;
        void visitCallInst(llvm::CallInst& CI);
        void visitImageSampleExplicitLod(llvm::CallInst& CI);
        void visitOpenCLEISPrintf(llvm::CallInst& CI);
    private:
        llvm::Value* getWidenImageCoordsArg(llvm::Value* Coords);
        void createCallAndReplace(llvm::CallInst& oldCallInst, llvm::StringRef newFuncName, std::vector<llvm::Value*>& args);
        uint64_t parseSampledImageTy(llvm::StructType* SampledImageTy);
        bool isSPVIR(llvm::StringRef funcName);

        llvm::Module* m_Module;
        llvm::IRBuilder<>* m_Builder;
        bool m_changed;
    };
}
