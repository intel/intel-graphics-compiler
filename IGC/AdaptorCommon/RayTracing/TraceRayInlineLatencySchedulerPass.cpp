/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This pass schedules latency of rayquery.Proceed() by moving SyncStackToShadowMemory instructions
/// to as far away as possible from TraceRaySyncProceed.
/// Note: any BB w/ suspendPoint is split first into <B3.0,B3.1>, <B7.0,B7.1> like below, where B3.1 and B7.1 holds suspendPoint.
/// Given CFG & DT like below. PTD is opposite of DT.
///     CFG                         DT
///     B0                          B0
///    /  \                    /    |    \
///  B10  B11                 B10  B3.0 B11
///   |    |                   |    |
///  B20   |                  B20  B3.1
///    \  /                      /  |    \
///     B3.0                   B40 B7.0   B41
///      |                          |   /  |  \
///     B3.1                      B7.1 B51 B6 B52
///    /   \
///  B40   B41
///   |    |  \
///   |   B51 B52
///   |    |   /
///   |    |  /
///   |    B6
///    \  /
///     B7.0
///      |
///     B7.1
/// Assume B0 has SyncStackToShadowMemory(). Note: q.Proceed() is in an upstream BB instead of B0.
/// The only possible candidates are: B0, B3.#, B7.#, which means,
///     we need to find the candidate node (CNode):
///     1) DT&PTD with B0
///     2) No suspendPoint between B0 and CNode.
/// Finally, we pick farthestBB among all candidate nodes.
/// Implementation wise, we only need to traverse all CNodes in DT (no other nodes) because
/// each CNode must be immediate DT&PDT to one of the other ones. No need to BFS whole DT!
/// Example Cases:
/// Case 1) If any node (B10,B11 or B20) between B0 and B3.0 has suspendPoint, then farthestBB is B0.
/// Case 2) If B3.1 has suspendPoint, then farthestBB is B3.0.
/// Case 3) If any node between B3.1 and B7.0 has suspendPoint, then farthestBB is B3.1.
/// Case 4) If B7.1 has suspendPoint, then farthestBB is B7.0.
/// Further Note: It doesn't matter if B0 itself is on a branch
/// because the branch itself is the subsystem we are working on then and B0 is still the srcBB.
/// If below happens, then, farthestBB will be B0 because B0 has no children in DT tree.
///  B0    |
///    \  /
///     B1
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "CrossingAnalysis.h"
#include "SplitAsyncUtils.h"
#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublicEnums.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Support/MathExtras.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "llvmWrapper/Transforms/Utils/LoopUtils.h"
#include "common/LLVMWarningsPop.hpp"

using namespace std;
using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

class TraceRayInlineLatencySchedulerPass : public FunctionPass
{
public:
    TraceRayInlineLatencySchedulerPass(): FunctionPass(ID)
    {
        initializeTraceRayInlineLatencySchedulerPassPass(*PassRegistry::getPassRegistry());
    }

    void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

    bool runOnFunction(Function &F) override;
    StringRef getPassName() const override
    {
        return "TraceRayInlineLatencySchedulerPass";
    }

    static char ID;

private:
    void split(RayQuerySyncStackToShadowMemory* Stk2SM, vector<Instruction*>& SPIs);
    void schedule(RayQuerySyncStackToShadowMemory* Stk2SM, vector<Instruction*>& SPIs);
    BasicBlock* findFarthestSafeBB(Instruction* src, SuspendCrossingInfo& checker);
};

char TraceRayInlineLatencySchedulerPass::ID = 0;

#define PASS_FLAG2 "tracerayinline-latency-scheduler-pass"
#define PASS_DESCRIPTION2 "schedule tracerayinline latency"
#define PASS_CFG_ONLY2 false
#define PASS_ANALYSIS2 false
IGC_INITIALIZE_PASS_BEGIN(TraceRayInlineLatencySchedulerPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(TraceRayInlineLatencySchedulerPass, PASS_FLAG2, PASS_DESCRIPTION2, PASS_CFG_ONLY2, PASS_ANALYSIS2)

bool TraceRayInlineLatencySchedulerPass::runOnFunction(Function &F)
{
    SmallVector<RayQuerySyncStackToShadowMemory*, 4> Stk2SMs;
    for (auto& I : instructions(F))
    {
        if (auto* stk2SM = dyn_cast<RayQuerySyncStackToShadowMemory>(&I))
            Stk2SMs.push_back(stk2SM);
    }

    unordered_map<RayQuerySyncStackToShadowMemory*, vector<Instruction*>> SPIs;
    for (auto& Stk2SM : Stk2SMs) {
        split(Stk2SM, SPIs[Stk2SM]);
    }

    for (auto& Stk2SM : Stk2SMs) {
        schedule(Stk2SM, SPIs[Stk2SM]);
    }

    return true;
}

void TraceRayInlineLatencySchedulerPass::split(RayQuerySyncStackToShadowMemory* Stk2SM, vector<Instruction*>& SPIs) {
    Function& F = *Stk2SM->getParent()->getParent();
    //to be conservative, we only return false when two constant arguments are not equal.
    auto mayAlias = [](Value* objIdx1, Value* objIdx2) {
        ConstantInt* CIdx1 = dyn_cast<ConstantInt>(objIdx1);
        ConstantInt* CIdx2 = dyn_cast<ConstantInt>(objIdx2);
        if (CIdx1 && CIdx2) {
            uint32_t idx1 = static_cast<uint32_t>(CIdx1->getZExtValue());
            uint32_t idx2 = static_cast<uint32_t>(CIdx1->getZExtValue());
            return (idx1 == idx2);
        }
        return true;
    };

    auto isSuspendPoint = [&](Instruction* inst) {
        //RayQuery Instructions
        if (auto* RQI = dyn_cast<RayQueryIntrinsicBase>(inst)) {
            return (RQI->getIntrinsicID() != GenISAIntrinsic::GenISA_SyncStackToShadowMemory &&
                (RQI->getIntrinsicID() == GenISAIntrinsic::GenISA_TraceRaySyncProceed || mayAlias(RQI->getQueryObjIndex(), Stk2SM->getQueryObjIndex())));
        }else if (auto* GI = dyn_cast<GenIntrinsicInst>(inst)){
            //The last two asyncRT intrinsics, IgnoreHit() and AcceptHitAndEndSearch(), are not suspendpoints
            //because they will cause shader to end immediately (instead of let shader to continue to end)
            return (isa<ContinuationHLIntrinsic>(GI) ||
                isa <TraceRayIntrinsic>(GI) ||
                isa <ReportHitHLIntrinsic>(GI));
        }else if (isa<CallInst>(inst) && !isa<IntrinsicInst>(inst)) {
            //We are conservative here because non-intrinsic function might call SuspendPoint instructions.
            return true;
        }
        return false;
    };

    for (auto& I : instructions(F)) {
        if (isSuspendPoint(&I)){
            SPIs.push_back(&I);
        }
    }

    for (auto* user : Stk2SM->users()) {
        if (Instruction* inst = dyn_cast<Instruction>(user))
            SPIs.push_back(inst);
    }

    if (SPIs.empty())
        return;

    for (auto* SPI : SPIs) {
        splitAround(SPI, VALUE_NAME("Cont"));
    }

    rewritePHIs(F);
}

void TraceRayInlineLatencySchedulerPass::schedule(RayQuerySyncStackToShadowMemory* Stk2SM, vector<Instruction*>& SPIs) {
    Function& F = *Stk2SM->getParent()->getParent();
    SuspendCrossingInfo checker(F, SPIs);
    BasicBlock* tgt = findFarthestSafeBB(Stk2SM, checker);
    Stk2SM->moveBefore(tgt->getTerminator());
}

//This function finds the farthest "safe" BB away from src BB.
//"safe" here means src can be safely moved to this BB.
//Note that "distance" here is not accurate because we use DT instead of CFG to traverse
BasicBlock* TraceRayInlineLatencySchedulerPass::findFarthestSafeBB(Instruction* src,
    SuspendCrossingInfo& checker)
{
    BasicBlock* srcBB = src->getParent();
    PostDominatorTree PDT(*srcBB->getParent());
    DominatorTree DT(*srcBB->getParent());
    BasicBlock* farthestBB = srcBB;
    DomTreeNodeBase<BasicBlock>* cNode = DT.getNode(srcBB);
    while (cNode) {
        DomTreeNodeBase<BasicBlock>* newCNode = nullptr;
        // TODO: change to node->children() once we move to llvm11
        for (auto nodeV = cNode->begin(); nodeV != cNode->end(); nodeV++) {
            auto* BBV = (*nodeV)->getBlock();
            if (PDT.dominates(BBV, srcBB) &&
                !checker.hasPathCrossingSuspendPoint(srcBB, BBV)) {
                newCNode = *nodeV;
                farthestBB = BBV;
                break;
            }
        }
        cNode = newCNode;
    }
    return farthestBB;
}


namespace IGC
{

Pass* createTraceRayInlineLatencySchedulerPass(void)
{
    return new TraceRayInlineLatencySchedulerPass();
}

} // namespace IGC
