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

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "FoldKnownWorkGroupSizes.h"
#include "../IGCPassSupport.h"
#include "../CodeGenPublic.h"
#include "../MetaDataApi/MetaDataApi.h"

#include "LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <llvm/IR/InstVisitor.h>
#include "LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"


namespace IGC
{
    class FoldKnownWorkGroupSizes : public llvm::FunctionPass, public llvm::InstVisitor<FoldKnownWorkGroupSizes>
    {
    private:
        static char ID;
    public:
        FoldKnownWorkGroupSizes() : FunctionPass(ID) {}
        bool runOnFunction(llvm::Function &F);
        void visitCallInst(llvm::CallInst &I);

        void getAnalysisUsage(llvm::AnalysisUsage &AU) const
        {
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

    };
    bool m_changed = false;
    char FoldKnownWorkGroupSizes::ID = 0;
}



using namespace llvm;
using namespace IGC;
using namespace IGCMD;

bool FoldKnownWorkGroupSizes::runOnFunction(Function &F)
{
    visit(F);
    return m_changed;
}

void FoldKnownWorkGroupSizes::visitCallInst(llvm::CallInst &I)
{
    Function* function = I.getParent()->getParent();
    Module* module = function->getParent();
    Function* calledFunction = I.getCalledFunction();
    if (calledFunction == nullptr)
    {
        return;
    }
    StringRef funcName = calledFunction->getName();
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();


    if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_OFFSET) &&
        ctx->getModuleMetaData()->compOpt.replaceGlobalOffsetsByZero)
    {
        if (calledFunction->getReturnType() == Type::getInt32Ty(module->getContext()))
        {
            ConstantInt* IntZero = ConstantInt::get(Type::getInt32Ty(module->getContext()), 0);
            I.replaceAllUsesWith(IntZero);
            // TODO: erase when patch token is not required
            //I.eraseFromParent();
            m_changed = true;
        }
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE))
    {
        auto itr = ctx->getMetaDataUtils()->findFunctionsInfoItem(I.getFunction());

        //Check function exists in the metadata
        if (itr == ctx->getMetaDataUtils()->end_FunctionsInfo())
            return;

        FunctionInfoMetaDataHandle funcMDHandle = itr->second;
        ThreadGroupSizeMetaDataHandle tgMD = funcMDHandle->getThreadGroupSize();
        //Check threadGroup has value
        if (!tgMD->hasValue())
            return;        

        IRBuilder<> IRB(&I);

        uint32_t Dims[] =
        {
            (uint32_t)tgMD->getXDim(),
            (uint32_t)tgMD->getYDim(),
            (uint32_t)tgMD->getZDim(),
        };
        auto *CV = ConstantDataVector::get(I.getContext(), Dims);

        auto *Dim = I.getArgOperand(0);
        auto *EE = IRB.CreateExtractElement(CV, Dim, "enqueuedLocalSize");

        I.replaceAllUsesWith(EE);
        // TODO: erase when patch token is not required
        //I.eraseFromParent();
        m_changed = true;
    }
}

namespace IGC
{
    llvm::FunctionPass* CreateFoldKnownWorkGroupSizes()
    {
        return new FoldKnownWorkGroupSizes();
    }
}

