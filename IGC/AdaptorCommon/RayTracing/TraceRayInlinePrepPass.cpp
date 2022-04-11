/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass prepares downstream RayQuery passes by lowering some specific intrinsics first.
/// Right now, it lowers Proceed only.
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class TraceRayInlinePrepPass : public FunctionPass
{
public:
    TraceRayInlinePrepPass(): FunctionPass(ID)
    {
        initializeTraceRayInlinePrepPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "TraceRayInlinePrepPass";
    }

    static char ID;

private:
    void lowerPI(Function& F);
};

char TraceRayInlinePrepPass::ID = 0;

#define PASS_FLAG2 "tracerayinline-prep-pass"
#define PASS_DESCRIPTION2 "prepare tracerayinline"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(TraceRayInlinePrepPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(TraceRayInlinePrepPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

/// Lower TraceRaySyncProceedHLIntrinsic to 2 intrinsics which will be scheduled by downstream schedulers.
/// bool status = TraceRaySyncProceedHLIntrinsic();
/// if(status)
/// ...
/// =========>
/// int retPI = TraceRaySyncProceedIntrinsic();
/// bool status = RayQuerySyncStackToShadowMemory(retPI);
/// if(status)
/// ...
/// TODO:   Right now, we don't use a separate GenISA_ShadowMemoryToSyncStack here, but we might want to do it later if necessary.
void TraceRayInlinePrepPass::lowerPI(Function& F)
{
    SmallVector<TraceRaySyncProceedHLIntrinsic*, 4> ProceedHLs;
    for (auto& I : instructions(F))
    {
        if (auto* PI = dyn_cast<TraceRaySyncProceedHLIntrinsic>(&I))
            ProceedHLs.push_back(PI);
    }

    if (ProceedHLs.empty())
        return;

    RTBuilder IRB(F.getContext(), *getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    for (auto* PIHL : ProceedHLs)
    {
        IRB.SetInsertPoint(PIHL->getNextNode());

        Function* proceedFunc = GenISAIntrinsic::getDeclaration(
            PIHL->getModule(), GenISAIntrinsic::GenISA_TraceRaySyncProceed);
        CallInst* PI = IRB.CreateCall(proceedFunc, PIHL->getQueryObjIndex());

        Function* stk2SMFunc = GenISAIntrinsic::getDeclaration(
            PIHL->getModule(),
            GenISAIntrinsic::GenISA_SyncStackToShadowMemory);
        Value* args[] = {
            PIHL->getQueryObjIndex(),
            PI
        };
        CallInst* stk2SM = IRB.CreateCall(stk2SMFunc, args);

        PIHL->replaceAllUsesWith(stk2SM);
        PIHL->eraseFromParent();
    }
    return;
}

bool TraceRayInlinePrepPass::runOnFunction(Function &F)
{
    SmallVector<RayQuerySyncStackToShadowMemory*, 4> Stk2SMs;
    lowerPI(F);

    return true;
}

namespace IGC
{

Pass* createTraceRayInlinePrepPass(void)
{
    return new TraceRayInlinePrepPass();
}

} // namespace IGC
