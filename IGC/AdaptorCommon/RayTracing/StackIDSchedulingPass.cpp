/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Some workloads may finish operating on RT Memory regions (e.g., SWStack)
/// early on in some of the shaders and do quite a bit more work after.
/// For example:
///
/// [shader("closesthit")]
/// void CHS(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
/// {
///     payload.value = (uint)attr.barycentrics.x;
///     // computations ...
///     // texture writes, etc.
///     StackIDRelease(); // inlined from raygen
/// }
///
/// Currently, we inject StackIDRelease calls at the termination of raygen
/// shaders which may then be inlined into other shaders.
///
/// This pass tries to schedule the StackIDRelease as early as possible to
/// free up the stack ID to allow further raygen shaders to execute sooner
/// to improve RT hardware utilization.
///
/// The above would then be:
///
/// [shader("closesthit")]
/// void CHS(inout Payload payload, in BuiltInTriangleIntersectionAttributes attr)
/// {
///     payload.value = (uint)attr.barycentrics.x;
///     StackIDRelease(); // inlined from raygen
///     // computations ...
///     // texture writes, etc.
/// }
///
//===----------------------------------------------------------------------===//

#include "RTBuilder.h"
#include "Compiler/IGCPassSupport.h"
#include "RayTracingAddressSpaceAliasAnalysis.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "llvmWrapper/Analysis/MemoryLocation.h"
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class StackIDSchedulingPass : public FunctionPass
{
public:
    StackIDSchedulingPass(): FunctionPass(ID)
    {
        initializeStackIDSchedulingPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(AnalysisUsage &AU) const override
    {
        AU.setPreservesCFG();
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<DominatorTreeWrapperPass>();
        AU.addRequired<PostDominatorTreeWrapperPass>();
        AU.addRequired<LoopInfoWrapperPass>();
        AU.addRequired<RayTracingAddressSpaceAAWrapperPass>();
    }

    bool runOnFunction(Function &M) override;
    StringRef getPassName() const override
    {
        return "StackIDSchedulingPass";
    }

    static char ID;
private:
    const DominatorTree* DT = nullptr;
    const PostDominatorTree* PDT = nullptr;
    const LoopInfo* LI = nullptr;
    RayTracingAddressSpaceAAResult* AA = nullptr;
private:
    // Maps blocks that contain at least one "hazard instruction" (i.e., an
    // instruction for which isSafe(I, SRI) returns false) to the last
    // instruction in that block that is a hazard.
    using HazardMap = std::unordered_map<const BasicBlock*, Instruction*>;

    ModRefInfo getModRefInfo(const CallBase *Call, const MemoryLocation &Loc);
    // returns true if the StackIDRelease could safely be scheduled across `I`.
    bool isSafe(const Instruction* I, const StackIDReleaseIntrinsic *SRI);
    // returns true if there is not path from `From` to its `IDom` that contains
    // a hazard
    bool isSafeToMove(
        const BasicBlock* From, const BasicBlock* IDom, const HazardMap& Hazards) const;
    BasicBlock* schedule(
        StackIDReleaseIntrinsic* SRI, const HazardMap& Hazards) const;
};

char StackIDSchedulingPass::ID = 0;
// Register pass to igc-opt
#define PASS_FLAG2 "stack-id-scheduling-pass"
#define PASS_DESCRIPTION2 "Schedule Stack ID Release earlier in shader"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(StackIDSchedulingPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(RayTracingAddressSpaceAAWrapperPass)
IGC_INITIALIZE_PASS_END(StackIDSchedulingPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

bool StackIDSchedulingPass::isSafeToMove(
    const BasicBlock* From, const BasicBlock* IDom, const HazardMap& Hazards) const
{
    SmallPtrSet<const BasicBlock*, 32> Visited;
    SmallVector<const BasicBlock*, 8> WorkList{ From };

    do {
        auto* BB = WorkList.pop_back_val();
        if (!Visited.insert(BB).second)
            continue;
        if (BB == IDom)
            continue;
        if (Hazards.count(BB) != 0)
            return false;
        WorkList.append(pred_begin(BB), pred_end(BB));
    } while (!WorkList.empty());

    return true;
}

BasicBlock* StackIDSchedulingPass::schedule(
    StackIDReleaseIntrinsic* SRI, const HazardMap& Hazards) const
{
    // Given that there could conceivably be an unreachable BB with a
    // StackIDRelease and all nodes in `DT` can dominate it, we just bail here.
    if (!DT->isReachableFromEntry(SRI->getParent()))
        return nullptr;

    auto getIDom = [&](const BasicBlock& BB) -> DomTreeNode*
    {
        auto* Node = DT->getNode(&BB);
        if (!Node)
            return nullptr;
        return Node->getIDom();
    };

    BasicBlock* CurBB = SRI->getParent();
    while (DomTreeNode* NextNode = getIDom(*CurBB))
    {
        // These conditions guarantee that the scheduled `SRI` will be executed
        // iff it executes in the original program.
        auto* NextBB = NextNode->getBlock();
        if (!PDT->dominates(SRI->getParent(), NextBB))
            return CurBB;
        // Ensure that we don't sink `SRI` into a loop since it must execute
        // at most once.
        if (LI->getLoopFor(NextBB))
            return CurBB;
        if (!isSafeToMove(CurBB, NextBB, Hazards))
            return CurBB;
        CurBB = NextBB;
    }

    return CurBB;
}

ModRefInfo StackIDSchedulingPass::getModRefInfo(
    const CallBase* Call, const MemoryLocation& Loc)
{
    AAQueryInfo AAQIP;
    return AA->getModRefInfo(Call, Loc, AAQIP);
}

bool StackIDSchedulingPass::isSafe(
    const Instruction* I, const StackIDReleaseIntrinsic* SRI)
{
    using IGCLLVM::LocationSize;

    if (!I->mayReadOrWriteMemory())
        return true;

    auto checkVal = [&](const Value* V)
    {
        if (!V->getType()->isPointerTy())
            return true;

        ModRefInfo Result = getModRefInfo(SRI, MemoryLocation(V, LocationSize::beforeOrAfterPointer()));
        return isNoModRef(Result);
    };

    // Examine all the argument to see if any may alias our `SRI`.
    for (auto& Op : I->operands())
    {
        if (!checkVal(Op))
            return false;
    }

    return checkVal(I);
}

bool StackIDSchedulingPass::runOnFunction(Function &F)
{
    auto *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    DT  = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();
    LI  = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
    AA  = &getAnalysis<RayTracingAddressSpaceAAWrapperPass>().getResult();

    RTBuilder RTB{ F.getContext(), *Ctx };
    // Insert a dummy release temporarily for the purpose of having something
    // to pass to alias analysis.
    RTB.SetInsertPoint(&*F.getEntryBlock().begin());
    auto* TempSRI = RTB.CreateStackIDRelease(RTB.getInt16(-1));

    bool Changed = false;

    HazardMap HazardBBs;
    // These are the releases that can potentially be scheduled. Any release
    // that already has hazards in its BB can't go anywhere but farther up in
    // that same BB.
    SmallVector<StackIDReleaseIntrinsic*, 4> Releases;
    for (auto& BB : F)
    {
        StackIDReleaseIntrinsic* CurRelease = nullptr;
        Instruction* CurHazard = nullptr;
        for (auto& I : reverse(BB))
        {
            if (auto* SRI = dyn_cast<StackIDReleaseIntrinsic>(&I))
            {
                // We only need to keep track of a single release in a given
                // BB because, if there were more than one, we'd already have
                // undefined behavior.
                if (SRI != TempSRI)
                    CurRelease = SRI;
            }
            else if (!isSafe(&I, TempSRI))
            {
                CurHazard = &I;
                break;
            }
        }

        if (CurRelease)
        {
            if (CurHazard)
            {
                RTB.SetInsertPoint(CurHazard->getNextNode());
                RTB.CreateStackIDRelease();
                CurRelease->eraseFromParent();
                Changed = true;
            }
            else
            {
                Releases.push_back(CurRelease);
            }
        }

        if (CurHazard)
            HazardBBs[&BB] = CurHazard;
    }

    // This doesn't count as `Changed` because we temporarily added this
    // instruction then deleted it leaving the function unchanged unless
    // otherwise modified.
    TempSRI->eraseFromParent();

    for (auto* SRI : Releases)
    {
        if (auto* NewBB = schedule(SRI, HazardBBs))
        {
            auto I = HazardBBs.find(NewBB);
            auto* MovePt = (I == HazardBBs.end()) ?
                &*NewBB->getFirstInsertionPt() : I->second->getNextNode();

            RTB.SetInsertPoint(MovePt);
            RTB.CreateStackIDRelease();
            SRI->eraseFromParent();
            Changed = true;
        }
    }

    return Changed;
}

namespace IGC
{

Pass* createStackIDSchedulingPass(void)
{
    return new StackIDSchedulingPass();
}

} // namespace IGC
