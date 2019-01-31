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
    //Value* callingInst = ;
    Module* module = function->getParent();
    Function* calledFunction = I.getCalledFunction();
    if (calledFunction == nullptr)
    {
        return;
    }
    StringRef funcName = calledFunction->getName();
    CodeGenContext* ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();


    if (funcName.equals("__builtin_IB_get_global_offset") && ctx->getModuleMetaData()->compOpt.replaceGlobalOffsetsByZero)
    {
        if (calledFunction->getReturnType() == Type::getInt32Ty(module->getContext()))
        {
            ConstantInt* IntZero = ConstantInt::get(Type::getInt32Ty(module->getContext()), 0);
            I.replaceAllUsesWith(IntZero);
            m_changed = true;
        }
        return;
    }
    else if (funcName.equals("__builtin_IB_get_enqueued_local_size") || funcName.equals("__builtin_IB_get_local_size"))
    {
        
        auto itr = ctx->getMetaDataUtils()->findFunctionsInfoItem(I.getParent()->getParent());

        //Check function exists in the metadata
        if (itr == ctx->getMetaDataUtils()->end_FunctionsInfo())
            return;

        FunctionInfoMetaDataHandle funcMDHandle = itr->second;
        ThreadGroupSizeMetaDataHandle tgMD = funcMDHandle->getThreadGroupSize();
        //Check threadGroup has value
        if (!tgMD->hasValue())
            return;        
        
        unsigned int dimension = (unsigned int)static_cast<ConstantInt*>(I.getArgOperand(0))->getZExtValue();
        ConstantInt *valueToReplaceWith = nullptr;
        if (dimension == 0)
        {
            if (tgMD->isXDimHasValue())
            {
                valueToReplaceWith = ConstantInt::get(Type::getInt32Ty(module->getContext()), tgMD->getXDim());
                I.replaceAllUsesWith(valueToReplaceWith);
                m_changed = true;

            }
        }
        else if (dimension == 1)
        {
            if (tgMD->isYDimHasValue())
            {
                valueToReplaceWith = ConstantInt::get(Type::getInt32Ty(module->getContext()), tgMD->getYDim());
                I.replaceAllUsesWith(valueToReplaceWith);
                m_changed = true;

            }
        } 
        else if (dimension == 2)
        {
            if (tgMD->isZDimHasValue())
            {
                valueToReplaceWith = ConstantInt::get(Type::getInt32Ty(module->getContext()), tgMD->getZDim());
                I.replaceAllUsesWith(valueToReplaceWith);
                m_changed = true;
            }
        }
        else
        {
            assert("Invalid thread group dimension");
        }
        return;
    }
}

namespace IGC
{
    llvm::FunctionPass* CreateFoldKnownWorkGroupSizes()
    {
        return new FoldKnownWorkGroupSizes();
    }
}

