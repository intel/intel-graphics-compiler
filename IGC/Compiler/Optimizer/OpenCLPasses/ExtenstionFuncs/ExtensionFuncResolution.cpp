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

#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionFuncResolution.hpp"
#include "Compiler/Optimizer/OpenCLPasses/ExtenstionFuncs/ExtensionFuncsAnalysis.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-extension-funcs-resolution"
#define PASS_DESCRIPTION "Resolves extension function"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ExtensionFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ExtensionFuncsResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ExtensionFuncsResolution::ID = 0;

ExtensionFuncsResolution::ExtensionFuncsResolution() : FunctionPass(ID), m_implicitArgs()
{
    initializeExtensionFuncsResolutionPass(*PassRegistry::getPassRegistry());
}

bool ExtensionFuncsResolution::runOnFunction(Function &F)
{
    m_changed = false;
    m_implicitArgs = ImplicitArgs(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
    visit(F);
    return m_changed;
}

void ExtensionFuncsResolution::visitCallInst(CallInst &CI)
{
    if (!CI.getCalledFunction())
    {
        return;
    }

    StringRef funcName = CI.getCalledFunction()->getName();
    Function& F = *(CI.getParent()->getParent());
    
    ImplicitArg::ArgType argType;
    if( funcName.equals(ExtensionFuncsAnalysis::VME_MB_BLOCK_TYPE) )
    {
        argType = ImplicitArg::VME_MB_BLOCK_TYPE;
    }
    else if( funcName.equals(ExtensionFuncsAnalysis::VME_SUBPIXEL_MODE) )
    {
        argType = ImplicitArg::VME_SUBPIXEL_MODE;
    }
    else if( funcName.equals(ExtensionFuncsAnalysis::VME_SAD_ADJUST_MODE) )
    {
        argType = ImplicitArg::VME_SAD_ADJUST_MODE;
    }
    else if( funcName.equals(ExtensionFuncsAnalysis::VME_SEARCH_PATH_TYPE) )
    {
        argType = ImplicitArg::VME_SEARCH_PATH_TYPE;
    } 
    else if (funcName.startswith(ExtensionFuncsAnalysis::VME_HELPER_GET_HANDLE)) {
        // Load from the opaque vme pointer and return the a vector with values.
        assert(CI.getNumArgOperands() == 1);
        IRBuilder<> builder(&CI);
        Type* retType = CI.getType();
        assert(retType->isVectorTy() || retType->isIntegerTy());
        PointerType* ptrType = PointerType::get(retType, 0);
        auto bitcastInst = builder.CreateBitCast(CI.getArgOperand(0), ptrType);
        auto ret = builder.CreateLoad(bitcastInst);
        CI.replaceAllUsesWith(ret);
        CI.eraseFromParent();
        return;
    }
    else if (funcName.startswith(ExtensionFuncsAnalysis::VME_HELPER_GET_AS)) {
        // Store the VME values and return an opaque vme pointer.
        assert(CI.getNumArgOperands() == 1);
        IRBuilder<> builder(&*CI.getParent()->getParent()->begin()->getFirstInsertionPt());
        Type* retType = CI.getType();
        Value* arg = CI.getArgOperand(0);
        assert(arg->getType()->isVectorTy() || arg->getType()->isIntegerTy());
        AllocaInst* allocaInst = builder.CreateAlloca(arg->getType());
        builder.SetInsertPoint(&CI);
        builder.CreateStore(arg, allocaInst);
        Value* bitcastInst = builder.CreateBitCast(allocaInst, retType);
        CI.replaceAllUsesWith(bitcastInst);
        CI.eraseFromParent();
        return;
    }
    else {
        // Non VME function, do nothing
        return;
    }

    Value* vmeRes = m_implicitArgs.getArgInFunc(F, argType);

    // Replace original VME call instruction with the appropriate argument, example:

    // Recieves:
    // call i32 @__builtin_IB_vme_subpixel_mode()

    // Creates:
    // %vmeSubpixelMode

    CI.replaceAllUsesWith(vmeRes);
    CI.eraseFromParent();

    m_changed = true;
}
