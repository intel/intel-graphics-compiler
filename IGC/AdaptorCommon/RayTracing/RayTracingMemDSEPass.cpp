/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass teaches IGC that raytracing memory like the SWStack is dead
/// if we can guarantee that a Stack ID Release is executed without any
/// intervening loads.
///
/// One example where this can occur is in response to payload sinking: we're
/// able to remove the load but we're left with a dead store that DSE doesn't
/// know is actually dead.
///
/// We may be able to remove this pass once we migrate to LLVM 12 which has
/// an enhanced DSE pass (it uses MemorySSA by default). We may be able to
/// mark the regions with a lifetime.end at release points.
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "MemRegionAnalysis.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/IR/InstVisitor.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RayTracingMemDSEPass : public FunctionPass, public InstVisitor<RayTracingMemDSEPass>
{
public:
    RayTracingMemDSEPass() : FunctionPass(ID)
    {
        initializeRayTracingMemDSEPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage& AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<AAResultsWrapperPass>();
        AU.addRequired<MemorySSAWrapperPass>();
        AU.addPreserved<MemorySSAWrapperPass>();
    }

    bool runOnFunction(Function& M) override;
    StringRef getPassName() const override
    {
        return "RayTracingMemDSEPass";
    }

    void visitStoreInst(StoreInst& SI);

    static char ID;
private:
    bool isDeadStore(const StoreInst& SI) const;
    void eraseStore(StoreInst& SI);
private:
    BatchAAResults* BAA = nullptr;
    MemorySSA* MSSA = nullptr;
    CodeGenContext* Ctx = nullptr;
    bool Changed = false;
};

char RayTracingMemDSEPass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "rt-mem-dse-pass"
#define PASS_DESCRIPTION2 "StackID Release kills stores to RT Memory"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(RayTracingMemDSEPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(RayTracingMemDSEPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

void RayTracingMemDSEPass::eraseStore(StoreInst& SI)
{
    MemorySSAUpdater Updater{ MSSA };
    if (auto* MA = MSSA->getMemoryAccess(&SI))
        Updater.removeMemoryAccess(MA);

    SI.eraseFromParent();
}

bool RayTracingMemDSEPass::isDeadStore(const StoreInst& SI) const
{
    auto* Def = MSSA->getMemoryAccess(&SI);
    if (!Def)
        return false;

    // This is inspired by isWriteAtEndOfFunction() in DeadStoreElimination

    SmallVector<MemoryAccess*, 4> WorkList;
    SmallPtrSet<MemoryAccess*, 8> Visited;
    auto PushMemUses = [&](MemoryAccess* Acc)
    {
        if (!Visited.insert(Acc).second)
            return;
        for (auto *U : Acc->users())
            WorkList.push_back(cast<MemoryAccess>(U));
    };
    PushMemUses(Def);

    for (unsigned i = 0; i < WorkList.size(); i++)
    {
        MemoryAccess* UseAccess = WorkList[i];
        if (isa<MemoryPhi>(UseAccess) || isa<MemoryUse>(UseAccess))
            return false;

        auto* UseInst = cast<MemoryDef>(UseAccess)->getMemoryInst();
        if (isa<StackIDReleaseIntrinsic>(UseInst))
            continue;
        if (isRefSet(BAA->getModRefInfo(UseInst, MemoryLocation::get(&SI))))
            return false;

        PushMemUses(UseAccess);
    }
    return true;
}

void RayTracingMemDSEPass::visitStoreInst(StoreInst& SI)
{
    auto Region = getRTRegion(SI.getPointerOperand(), *Ctx->getModuleMetaData());
    if (!Region)
        return;
    if (*Region != RTMemRegion::SWStack && *Region != RTMemRegion::SWHotZone)
        return;

    if (isDeadStore(SI))
    {
        eraseStore(SI);
        Changed = true;
    }
}

bool RayTracingMemDSEPass::runOnFunction(Function& F)
{
    Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    MSSA = &getAnalysis<MemorySSAWrapperPass>().getMSSA();
    auto &AA = getAnalysis<AAResultsWrapperPass>().getAAResults();
    BatchAAResults BatchAA{ AA };
    BAA = &BatchAA;
    Changed = false;

    visit(F);

    return Changed;
}

namespace IGC
{

Pass* createRayTracingMemDSEPass(void)
{
    return new RayTracingMemDSEPass();
}

} // namespace IGC
