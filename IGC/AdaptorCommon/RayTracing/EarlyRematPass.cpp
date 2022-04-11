/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// LSC L1$ hit rates are important for raytracing throughput.  We want to
/// minimize the amount of live (spilled) data between the various
/// continuations of a shader (useful even if we set caching controls to skip
/// L1 for the spill data).
///
/// This is meant as a very simple first pass prior to shader splitting where
/// we rematerialize values closer to their uses such that th live range
/// doesn't cross a TraceRay() call.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class EarlyRematPass : public FunctionPass
{
public:
    EarlyRematPass() : FunctionPass(ID)
    {
        initializeEarlyRematPassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "EarlyRematPass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }

    static char ID;
private:
    bool Changed;
};

char EarlyRematPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "early-remat"
#define PASS_DESCRIPTION "Do simple remats prior to shader splitting"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(EarlyRematPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(EarlyRematPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool EarlyRematPass::runOnFunction(Function &F)
{
    Changed = false;

    SmallVector<Instruction*, 8> RematInsts;

    for (auto& I : instructions(F))
    {
        auto* GII = dyn_cast<GenIntrinsicInst>(&I);

        if (!GII)
            continue;

        // These values are all read either directly from the global pointer
        // or through indirection to RTStack data via the global pointer.
        // It's clear that these will reduce spills as the entire intrinsic
        // can be recomputed in the continuation without needing to spill
        // any of the operands.
        //
        // TODO: probably want to grow this with additional cheap nomem
        // operations.
        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_DispatchRayIndex:
        case GenISAIntrinsic::GenISA_RuntimeValue:
        case GenISAIntrinsic::GenISA_DispatchDimensions:
        case GenISAIntrinsic::GenISA_GlobalRootSignatureValue:
            // This is usually profitable but might want to do more analysis
            // here if we see many cases where it doesn't pan out.
        case GenISAIntrinsic::GenISA_LocalRootSignatureValue:
            RematInsts.push_back(GII);
            break;
        default:
            break;
        }
    }

    for (auto* I : RematInsts)
    {
        SmallVector<Use*, 4> Uses;
        for (auto& U : I->uses())
            Uses.push_back(&U);

        for (auto *U : Uses)
        {
            auto *User = cast<Instruction>(U->getUser());
            Instruction* IP = nullptr;
            if (auto *PN = dyn_cast<PHINode>(User))
            {
                auto *InBB = PN->getIncomingBlock(*U);
                IP = InBB->getTerminator();
            }
            else
            {
                IP = User;
            }

            auto* NewI = I->clone();
            NewI->insertBefore(IP);
            U->set(NewI);
            Changed = true;
        }

        if (I->use_empty())
            I->eraseFromParent();
    }

    return Changed;
}

namespace IGC
{

Pass* createEarlyRematPass(void)
{
    return new EarlyRematPass();
}

} // namespace IGC
