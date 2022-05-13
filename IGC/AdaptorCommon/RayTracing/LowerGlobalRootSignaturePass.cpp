/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// GenISA_GlobalRootSignatureValue is lowered into loads at an offset from
/// the base of the RayDispatchGlobalData.
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"
#include <vector>
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;

class LowerGlobalRootSignaturePass : public FunctionPass
{
public:
    LowerGlobalRootSignaturePass(): FunctionPass(ID)
    {
        initializeLowerGlobalRootSignaturePassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "LowerGlobalRootSignaturePass";
    }

    static char ID;
};

char LowerGlobalRootSignaturePass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG1 "lower-global-root-signature-pass"
#define PASS_DESCRIPTION1 "lowering global root signature"
#define PASS_CFG_ONLY1 false
#define PASS_ANALYSIS1 false
IGC_INITIALIZE_PASS_BEGIN(LowerGlobalRootSignaturePass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerGlobalRootSignaturePass, PASS_FLAG1, PASS_DESCRIPTION1, PASS_CFG_ONLY1, PASS_ANALYSIS1)

bool LowerGlobalRootSignaturePass::runOnFunction(Function &F)
{
    auto* pCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    RTBuilder RTB(F.getContext(), *pCtx);

    bool Changed = false;

    for (auto II = inst_begin(&F), IE = inst_end(&F); II != IE; /* empty */)
    {
        auto* GII = dyn_cast<GenIntrinsicInst>(&*II++);
        if (!GII || GII->getIntrinsicID() != GenISAIntrinsic::GenISA_GlobalRootSignatureValue)
            continue;

        // Ptr = RTGlobalsBase + sizeof(RTGlobals) + ValueIdx*4;
        RTB.SetInsertPoint(GII);
        auto* GlobalPtr = RTB.getGlobalBufferPtr();
        uint32_t Addrspace = GlobalPtr->getType()->getPointerAddressSpace();
        GlobalPtr = RTB.CreateBitCast(GlobalPtr, RTB.getInt8PtrTy(Addrspace));
        auto *RootSigBase =
            RTB.CreateGEP(GlobalPtr, RTB.getInt64(sizeof(RayDispatchGlobalData)));
        auto* Ptr = RTB.CreateGEP(
            RootSigBase,
            RTB.CreateShl(GII->getOperand(0), 2));
        Ptr = RTB.CreateBitCast(Ptr, PointerType::get(GII->getType(), Addrspace));
        auto* LI = RTB.CreateLoad(Ptr);
        RTBuilder::setInvariantLoad(LI);
        LI->takeName(GII);
        GII->replaceAllUsesWith(LI);
        GII->eraseFromParent();
        Changed = true;
    }

    return Changed;
}

namespace IGC
{

Pass* createLowerGlobalRootSignaturePass(void)
{
    return new LowerGlobalRootSignaturePass();
}

} // namespace IGC
