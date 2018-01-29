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
#ifndef __GEN_UPDATE_CB__
#define __GEN_UPDATE_CB__

#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

void initializeGenUpdateCBPass(llvm::PassRegistry &);

namespace IGC
{

    class GenUpdateCB : public FunctionPass
    {
    public:
        static char ID;
        GenUpdateCB() : FunctionPass(ID)
        {
            initializeGenUpdateCBPass(*PassRegistry::getPassRegistry());
        }
        virtual llvm::StringRef getPassName() const { return "GenUpdateCB"; }
        virtual bool runOnFunction(Function &F);
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
        }
    private:
        bool isConstantBufferLoad(LoadInst* inst, unsigned &bufId);
        bool allSrcConstantOrImm(Instruction* inst);
        bool updateCbAllowedInst(Instruction* inst);
        void InsertInstTree(Instruction *inst, Instruction *pos);
        Instruction* CreateModule(Module* newModule);
        llvm::SetVector<llvm::Value*> m_CbUpdateMap;
        CodeGenContext *m_ctx;
        ValueToValueMapTy vmap;
        Module* m_ConstantBufferReplaceShaderPatterns = nullptr;
        uint m_ConstantBufferReplaceShaderPatternsSize = 0;
        uint m_ConstantBufferUsageMask = 0;
        uint m_maxCBcases = 128;
    };
}

#endif
