/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
        CodeGenContext* ctx = nullptr;
        bool RequirePayloadHeader = true;
    public:
        FoldKnownWorkGroupSizes() : FunctionPass(ID) {}
        bool runOnFunction(llvm::Function& F);
        void visitCallInst(llvm::CallInst& I);

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const
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

bool FoldKnownWorkGroupSizes::runOnFunction(Function& F)
{
    ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    RequirePayloadHeader = ctx->m_DriverInfo.RequirePayloadHeader();
    visit(F);
    return m_changed;
}

void FoldKnownWorkGroupSizes::visitCallInst(llvm::CallInst& I)
{
    Function* function = I.getParent()->getParent();
    Module* module = function->getParent();
    Function* calledFunction = I.getCalledFunction();
    if (calledFunction == nullptr)
    {
        return;
    }
    StringRef funcName = calledFunction->getName();

    if (funcName.equals(WIFuncsAnalysis::GET_GLOBAL_OFFSET) &&
        ctx->getModuleMetaData()->compOpt.replaceGlobalOffsetsByZero)
    {
        if (calledFunction->getReturnType() == Type::getInt32Ty(module->getContext()))
        {
            ConstantInt* IntZero = ConstantInt::get(Type::getInt32Ty(module->getContext()), 0);
            I.replaceAllUsesWith(IntZero);
            if (!RequirePayloadHeader)
                I.eraseFromParent();
            m_changed = true;
        }
    }
    else if (funcName.equals(WIFuncsAnalysis::GET_ENQUEUED_LOCAL_SIZE))
    {
        auto Dims = IGCMetaDataHelper::getThreadGroupDims(
            *ctx->getMetaDataUtils(),
            I.getFunction());

        if (!Dims)
            return;

        IRBuilder<> IRB(&I);

        auto* CV = ConstantDataVector::get(I.getContext(), *Dims);

        auto* Dim = I.getArgOperand(0);
        auto* EE = IRB.CreateExtractElement(CV, Dim, "enqueuedLocalSize");

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

