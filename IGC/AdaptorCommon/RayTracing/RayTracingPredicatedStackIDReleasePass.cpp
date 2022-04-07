/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "RTBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "llvm/Transforms/Utils/SSAUpdater.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RayTracingPredicatedStackIDReleasePass : public FunctionPass
{
public:
    RayTracingPredicatedStackIDReleasePass() : FunctionPass(ID)
    {
        initializeRayTracingPredicatedStackIDReleasePassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(Function& M) override;
    StringRef getPassName() const override
    {
        return "RayTracingPredicatedStackIDReleasePass";
    }

    static char ID;
};

static bool isKnownFalse(const Value* V)
{
    if (auto* CI = dyn_cast<ConstantInt>(V))
    {
        if (CI->isZero())
            return true;
    }
    return false;
}

char RayTracingPredicatedStackIDReleasePass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "rt-predicated-stack-id-release"
#define PASS_DESCRIPTION2 "Emit single predicated release at end of shader"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(RayTracingPredicatedStackIDReleasePass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RayTracingPredicatedStackIDReleasePass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

bool RayTracingPredicatedStackIDReleasePass::runOnFunction(Function& F)
{
    auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    SmallVector<StackIDReleaseIntrinsic*, 4> Releases;
    SmallVector<ReturnInst*, 4> Returns;

    for (auto& I : instructions(F))
    {
        if (auto *SIR = dyn_cast<StackIDReleaseIntrinsic>(&I))
            Releases.push_back(SIR);
        else if (auto* RI = dyn_cast<ReturnInst>(&I))
            Returns.push_back(RI);
    }

    if (Releases.empty())
        return false;

    RTBuilder RTB(F.getContext(), *Ctx);

    SSAUpdater Updater;
    // Since this is run post-legalization, we don't want any phis with i1 type.
    Updater.Initialize(RTB.getInt16Ty(), VALUE_NAME("StackIDReleasePred"));
    Updater.AddAvailableValue(&F.getEntryBlock(), RTB.getInt16(0));

    for (auto* SIR : Releases)
    {
        RTB.SetInsertPoint(SIR);
        Updater.AddAvailableValue(
            SIR->getParent(),
            RTB.CreateZExt(SIR->getPredicate(), RTB.getInt16Ty()));
        SIR->eraseFromParent();
    }

    for (auto* RI : Returns)
    {
        Value* Pred = Updater.GetValueAtEndOfBlock(RI->getParent());
        if (!isKnownFalse(Pred))
        {
            RTB.SetInsertPoint(RI);
            Pred = RTB.CreateTrunc(Pred, RTB.getInt1Ty());
            RTB.CreateStackIDRelease(nullptr, Pred);
        }
    }

    return true;
}

namespace IGC
{

Pass* createRayTracingPredicatedStackIDReleasePass(void)
{
    return new RayTracingPredicatedStackIDReleasePass();
}

} // namespace IGC
