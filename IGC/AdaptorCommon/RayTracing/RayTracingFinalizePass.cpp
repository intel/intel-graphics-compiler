/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This is a simple pass intended to do any final transformations before going
/// to the OptimizeIR stage.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "iStdLib/utility.h"
#include "common/LLVMUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/Local.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Dominators.h>

#include <llvmWrapper/Support/Alignment.h>

#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RayTracingFinalizePass : public ModulePass
{
public:
    RayTracingFinalizePass() : ModulePass(ID)
    {
        initializeRayTracingFinalizePassPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override;
    StringRef getPassName() const override
    {
        return "RayTracingFinalizePass";
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<DominatorTreeWrapperPass>();
    }

    static char ID;
};

char RayTracingFinalizePass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "raytracing-finalize"
#define PASS_DESCRIPTION "Final transformations prior to optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayTracingFinalizePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(RayTracingFinalizePass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

static void removeDups(Value *&V, Instruction* I, BasicBlock& EntryBB)
{
    if (V)
    {
        I->replaceAllUsesWith(V);
        I->eraseFromParent();
    }
    else
    {
        I->moveBefore(&*EntryBB.getFirstInsertionPt());
        V = I;
    }
}

bool RayTracingFinalizePass::runOnModule(Module &M)
{
    auto *Ctx = static_cast<RayDispatchShaderContext*>(
        getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
    for (auto& F : M)
    {
        if (F.isDeclaration())
            continue;

        Ctx->setShaderHash(&F);

        auto& EntryBB = F.getEntryBlock();
        auto& DL = M.getDataLayout();

        auto& DT = getAnalysis<DominatorTreeWrapperPass>(F).getDomTree();

        BasicBlock* AsyncBB = nullptr;
        SmallVector<Instruction*, 8> Asyncs;

        BasicBlock* HotZoneBB = nullptr;
        SmallVector<Instruction*, 8> HotZones;

        Value* AsyncStackID        = nullptr;
        Value* GlobalBufferPointer = nullptr;
        Value* LocalBufferPointer  = nullptr;

        for (auto II = inst_begin(&F), IE = inst_end(&F); II != IE; /* empty */)
        {
            Instruction& I = *II++;
            if (auto * LI = dyn_cast<LoadInst>(&I))
            {
                // Temporary WA to ensure we don't page fault on unaligned
                // acceses.
                uint32_t Align = (uint32_t)LI->getAlignment();
                if (Align == 0)
                    Align = (uint32_t)DL.getTypeAllocSize(LI->getType());

                if (Align >= 8)
                    LI->setAlignment(IGCLLVM::getCorrectAlign(4));
            }
            else if (auto * SI = dyn_cast<StoreInst>(&I))
            {
                // Temporary WA to ensure we don't page fault on unaligned
                // acceses.
                uint32_t Align = (uint32_t)SI->getAlignment();
                if (Align == 0)
                    Align = (uint32_t)DL.getTypeAllocSize(
                        SI->getValueOperand()->getType());

                if (Align >= 8)
                    SI->setAlignment(IGCLLVM::getCorrectAlign(4));
}
            else if (auto * GII = dyn_cast<GenIntrinsicInst>(&I))
            {
                // These will be part of the payload.  Move them to the entry
                // to guarantee they are CSE'd to a single call each.
                switch (GII->getIntrinsicID())
                {
                case GenISAIntrinsic::GenISA_ContinuationSignpost:
                {
                    auto* Signpost = cast<ContinuationSignpostIntrinsic>(GII);
                    Signpost->replaceAllUsesWith(Signpost->getFrameAddr());
                    Signpost->eraseFromParent();
                    break;
                }
                case GenISAIntrinsic::GenISA_PayloadPtr:
                {
                    auto* Payload = cast<PayloadPtrIntrinsic>(GII);
                    Payload->replaceAllUsesWith(Payload->getPayloadPtr());
                    Payload->eraseFromParent();
                    break;
                }
                case GenISAIntrinsic::GenISA_AsyncStackID:
                    removeDups(AsyncStackID, GII, EntryBB);
                    break;
                case GenISAIntrinsic::GenISA_GlobalBufferPointer:
                    removeDups(GlobalBufferPointer, GII, EntryBB);
                    break;
                case GenISAIntrinsic::GenISA_LocalBufferPointer:
                    removeDups(LocalBufferPointer, GII, EntryBB);
                    break;
                case GenISAIntrinsic::GenISA_AsyncStackPtr:
                    Asyncs.push_back(GII);
                    if (!AsyncBB)
                    {
                        AsyncBB = GII->getParent();
                    }
                    else
                    {
                        AsyncBB = DT.findNearestCommonDominator(
                            AsyncBB, GII->getParent());
                    }
                    break;
                case GenISAIntrinsic::GenISA_SWHotZonePtr:
                    HotZones.push_back(GII);
                    if (!HotZoneBB)
                    {
                        HotZoneBB = GII->getParent();
                    }
                    else
                    {
                        HotZoneBB = DT.findNearestCommonDominator(
                            HotZoneBB, GII->getParent());
                    }
                    break;
                default:
                    break;
                }
            }
        }

        if (AsyncBB)
        {
            RTBuilder RTB(&*AsyncBB->getFirstInsertionPt(), *Ctx);

            Value* AsyncStackPtr =
                RTB.getAsyncStackPointer(true);

            for (auto* I : Asyncs)
            {
                I->replaceAllUsesWith(AsyncStackPtr);
                I->eraseFromParent();
            }
        }

        if (HotZoneBB)
        {
            RTBuilder RTB(&*HotZoneBB->getFirstInsertionPt(), *Ctx);

            Value* SWHotZonePtr = RTB.getSWHotZonePointer(true);

            for (auto* I : HotZones)
            {
                I->replaceAllUsesWith(SWHotZonePtr);
                I->eraseFromParent();
            }
        }
    }

    return true;
}

namespace IGC
{

Pass* createRayTracingFinalizePass(void)
{
    return new RayTracingFinalizePass();
}

} // namespace IGC
