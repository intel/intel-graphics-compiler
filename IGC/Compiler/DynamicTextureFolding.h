/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
            GFXSURFACESTATE_SURFACETYPE_RESERVED = 0x6,
            GFXSURFACESTATE_SURFACETYPE_NULL
        };
        virtual bool runOnFunction(llvm::Function& F) override;
        bool doFinalization(llvm::Module&) override;
        void visitCallInst(llvm::CallInst& I);
    private:
        CodeGenContext* m_context;
        std::unordered_map<unsigned, SResInfoFoldingOutput> m_ResInfoFoldingOutput;
        void FoldSingleTextureValue(llvm::CallInst& I);
        template<typename ContextT>
        void copyResInfoData(ContextT* pShaderCtx);
        void FoldResInfoValue(llvm::GenIntrinsicInst* pCall);
        llvm::Value* ShiftByLOD(llvm::Instruction* pCall, unsigned int dimension, llvm::Value* val);
    };
}


