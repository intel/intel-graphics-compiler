/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WIFuncs/WIFuncsAnalysis.hpp"
#include "FoldKnownWorkGroupSizes.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include <llvm/IR/InstVisitor.h>
#include "LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGCMD;

namespace IGC
{
    class FoldKnownWorkGroupSizes : public llvm::FunctionPass, public llvm::InstVisitor<FoldKnownWorkGroupSizes>
    {
    private:
        CodeGenContext* ctx = nullptr;
        bool RequirePayloadHeader = true;
    public:
        static char ID;
        FoldKnownWorkGroupSizes() : FunctionPass(ID) {}
        bool runOnFunction(llvm::Function& F) override;
        void visitCallInst(llvm::CallInst& I);

        llvm::StringRef getPassName() const override {
            return "FoldKnownWorkGroupSizes";
        }

        void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<IGC::CodeGenContextWrapper>();
        }

    };
    bool m_changed = false;
    char FoldKnownWorkGroupSizes::ID = 0;

#define PASS_FLAG "igc-fold-workgroup-sizes"
#define PASS_DESCRIPTION "Fold global offset and enqueued local sizes"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
    IGC_INITIALIZE_PASS_BEGIN(FoldKnownWorkGroupSizes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
    IGC_INITIALIZE_PASS_END(FoldKnownWorkGroupSizes, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
}

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

