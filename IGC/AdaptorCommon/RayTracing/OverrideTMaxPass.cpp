/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Override raytracing TMax value for testing purposes.
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

class OverrideTMaxPass : public FunctionPass
{
public:
    OverrideTMaxPass(uint32_t OverrideValue = 0) :
        OverrideValue(OverrideValue),
        FunctionPass(ID)
    {
        initializeOverrideTMaxPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "OverrideTMaxPass";
    }

    static char ID;
private:
    uint32_t OverrideValue = 0;
};

char OverrideTMaxPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG1 "override-tmax"
#define PASS_DESCRIPTION1 "override tmax for testing purposes"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(OverrideTMaxPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_END(OverrideTMaxPass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

bool OverrideTMaxPass::runOnFunction(Function &F)
{
    if (OverrideValue == 0)
        return false;

    IRBuilder<> IRB(F.getContext());
    auto updateTMax = [&](auto* GII)
    {
        auto* Val = ConstantFP::get(
            IRB.getFloatTy(), static_cast<double>(OverrideValue));
        GII->setTMax(Val);

        return true;
    };

    bool Changed = false;
    for (auto& I : instructions(F))
    {
        auto* GII = dyn_cast<GenIntrinsicInst>(&I);
        if (!GII)
            continue;

        switch (GII->getIntrinsicID())
        {
        case GenISAIntrinsic::GenISA_TraceRayInlineHL:
            Changed |= updateTMax(cast<TraceRayInlineHLIntrinsic>(GII));
            break;
        default:
            break;
        }
    }

    return Changed;
}

namespace IGC
{

Pass* createOverrideTMaxPass(uint32_t OverrideValue)
{
    return new OverrideTMaxPass(OverrideValue);
}

} // namespace IGC
