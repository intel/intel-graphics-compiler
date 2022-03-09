/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Replace specified raytracing intrinsics with implicit args.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstVisitor.h"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

//////////////////////////////////////////////////////////////////////////
//
// Now that we have implicit args, replace the intrinsics
class RayTracingIntrinsicResolution : public FunctionPass, public InstVisitor<RayTracingIntrinsicResolution>
{
public:
    RayTracingIntrinsicResolution();
    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "RayTracingIntrinsicResolution";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<MetaDataUtilsWrapper>();
        AU.addRequired<CodeGenContextWrapper>();
    }

    void visitCallInst(CallInst &CI);

    static char ID;
private:
    Argument* getImplicitArg(Function *F, ImplicitArg::ArgType argType);
private:
    ImplicitArgs m_implicitArgs;
    bool Changed;
    CodeGenContext* m_CGCtx = nullptr;
};

#define PASS_FLAG "raytracing-intrinsic-resolution"
#define PASS_DESCRIPTION "replace intrinsics with implicit args"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingIntrinsicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_END(RayTracingIntrinsicResolution, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

Argument* RayTracingIntrinsicResolution::getImplicitArg(
    Function *F, ImplicitArg::ArgType argType)
{
    return m_implicitArgs.getImplicitArg(*F, argType);
}

RayTracingIntrinsicResolution::RayTracingIntrinsicResolution() : FunctionPass(ID) {
    initializeRayTracingIntrinsicResolutionPass(*PassRegistry::getPassRegistry());
}

char RayTracingIntrinsicResolution::ID = 0;

void RayTracingIntrinsicResolution::visitCallInst(CallInst &CI)
{
    Value *Arg = nullptr;
    Function *F = CI.getFunction();
    if (auto *GII = dyn_cast<GenIntrinsicInst>(&CI))
    {
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_GlobalBufferPointer:
            Arg = getImplicitArg(F, ImplicitArg::RT_GLOBAL_BUFFER_POINTER);
            break;
        case GenISAIntrinsic::GenISA_LocalBufferPointer:
            Arg = getImplicitArg(F, ImplicitArg::RT_LOCAL_BUFFER_POINTER);
            break;
        case GenISAIntrinsic::GenISA_AsyncStackID:
            Arg = getImplicitArg(F, ImplicitArg::RT_STACK_ID);
            break;
        case GenISAIntrinsic::GenISA_InlinedData:
        {
            // the global and local pointer are both passed in so the argument
            // is a vector of two pointers.
            Arg = getImplicitArg(F, ImplicitArg::RT_INLINED_DATA);
            IRBuilder<> IRB(&CI);
            Arg = IRB.CreateExtractElement(Arg, CI.getOperand(0));
            break;
        }
        default:
            break;
        }
    }

    if (!Arg)
        return;

    IRBuilder<> IRB(&CI);
    Arg = IRB.CreateBitCast(Arg, CI.getType());

    CI.replaceAllUsesWith(Arg);
    CI.eraseFromParent();

    Changed = true;
}

bool RayTracingIntrinsicResolution::runOnFunction(Function &F)
{
    auto *pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo())
        return false;

    Changed = false;
    m_implicitArgs = ImplicitArgs(
        F,
        pMdUtils);
    visit(F);

    return Changed;
}

namespace IGC
{
    Pass* createRayTracingIntrinsicResolutionPass()
    {
        return new RayTracingIntrinsicResolution();
    }
}
