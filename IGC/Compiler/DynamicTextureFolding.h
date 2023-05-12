/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Dominators.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class DynamicTextureFolding : public llvm::FunctionPass, public llvm::InstVisitor<DynamicTextureFolding>
    {
    public:
        static char ID;

        DynamicTextureFolding();
        ~DynamicTextureFolding() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "Dynamic Texture Folding";
        }
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }
        enum GFXSURFACESTATE_SURFACETYPE
        {
            GFXSURFACESTATE_SURFACETYPE_1D = 0x0,
            GFXSURFACESTATE_SURFACETYPE_2D,
            GFXSURFACESTATE_SURFACETYPE_3D,
            GFXSURFACESTATE_SURFACETYPE_CUBE,
            GFXSURFACESTATE_SURFACETYPE_BUFFER,
            GFXSURFACESTATE_SURFACETYPE_STRBUF,
            GFXSURFACESTATE_SURFACETYPE_SCRATCH,
            GFXSURFACESTATE_SURFACETYPE_NULL
        };
        virtual bool runOnFunction(llvm::Function& F) override;
        bool doFinalization(llvm::Module&) override;
        void visitCallInst(llvm::CallInst& I);
    private:
        CodeGenContext* m_context = nullptr;
        Module* m_module = nullptr;
        std::unordered_map<unsigned, SResInfoFoldingOutput> m_ResInfoFoldingOutput;
        void FoldSingleTextureValue(llvm::CallInst& I);
        template<typename ContextT>
        void copyResInfoData(ContextT* pShaderCtx);
        void FoldResInfoValue(llvm::GenIntrinsicInst* pCall);
        llvm::Value* ShiftByLOD(llvm::Instruction* pCall, unsigned int dimension, llvm::Value* val);
        SmallVector<Instruction*,4> InstsToRemove;
    };
}


