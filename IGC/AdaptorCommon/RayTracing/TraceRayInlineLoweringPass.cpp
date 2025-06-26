/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RayTracingInterface.h"
#include "RTBuilder.h"
#include "RTStackFormat.h"
#include "common/LLVMUtils.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DepthFirstIterator.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include <vector>
#include <unordered_map>
#include "Probe/Assertion.h"

using namespace IGC;
using namespace llvm;
using namespace std;
using namespace RTStackFormat;

//Lowering pass for Synchronous raytracing intrinsics known as TraceRayInline/RayQuery
class TraceRayInlineLoweringPass : public FunctionPass
{
    LoopInfo* LI = nullptr;
public:
    TraceRayInlineLoweringPass() : FunctionPass(ID) {
        initializeTraceRayInlineLoweringPassPass(*PassRegistry::getPassRegistry());
    }
    bool runOnFunction(Function& F) override;
    llvm::StringRef getPassName() const override
    {
        return "TraceRayInlineLoweringPass";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
        AU.addRequired<LoopInfoWrapperPass>();
    }
    static char ID;
private:
    //m_ShMemRTStacks is an array of RTStackFormat::RTStack/SMStack in ShadowMemory
    //m_ShMemRTCtrls  is an array of RTStackFormat::RTCtrl in ShadowMemory
    //together, they are RayQueryObjects[n]
    //RTStack2/SMStack2 m_ShMemRTStacks[n]
    AllocaInst* m_ShMemRTStacks = nullptr;
    //RayQueryStateInfo m_ShMemRTCtrls[n]
    AllocaInst* m_ShMemRTCtrls = nullptr;
    CodeGenContext* m_CGCtx = nullptr;
    bool singleRQMemRayStore = false;
    //if there is only one Proceed and it's not in a loop, then, we only need to prepare data for Proceed() once
    //where it's for initialization
    //FIXME: hack code, fix this hack in stage 2.
    bool singleRQProceed = true;

    bool LowerAllocateRayQuery(Function& F, unsigned numProceeds);
    void LowerTraceRayInline(Function& F);
    void LowerTraceRaySyncProceedIntrinsic(Function& F);
    void LowerSyncStackToShadowMemory(Function& F);
    void LowerAbort(Function& F);
    void LowerCommittedStatus(Function& F);
    void LowerCandidateType(Function& F);
    void LowerRayInfo(Function& F);
    void LowerCommitNonOpaqueTriangleHit(Function& F);
    void LowerCommitProceduralPrimitiveHit(Function& F);

    //return m_ShMemRTCtrls[index]
    GetElementPtrInst* getShMemRTCtrl(RTBuilder& builder, unsigned queryIndex) {
        return getShMemRTCtrl(builder, builder.getInt32(queryIndex));
    }

    //return m_ShMemRTCtrls[index]
    GetElementPtrInst* getShMemRTCtrl(RTBuilder& builder, Value* queryIndex) {
        return GetElementPtrInst::Create(m_ShMemRTCtrls->getAllocatedType(),
            m_ShMemRTCtrls, { builder.getInt32(0), queryIndex }, VALUE_NAME("&shadowMem.RTCtrl"), &(*builder.GetInsertPoint()));
    }

    //return rtStacks[index]
    RTBuilder::SyncStackPointerVal* getShMemRayQueryRTStack(RTBuilder& builder, unsigned queryIndex) {
        return getShMemRayQueryRTStack(builder, builder.getInt32(queryIndex));
    }

    //return rtStacks[index]
    RTBuilder::SyncStackPointerVal* getShMemRayQueryRTStack(RTBuilder& builder, Value* queryIndex) {
        return static_cast<RTBuilder::SyncStackPointerVal*>(
            builder.CreateGEP(m_ShMemRTStacks->getAllocatedType(),
                m_ShMemRTStacks, { builder.getInt32(0), queryIndex }, VALUE_NAME("&shadowMem.RTStack")));
    }

    std::pair<BasicBlock*, BasicBlock*> branchOnPotentialHitDone(
        RTBuilder &IRB,
        RayQueryIntrinsicBase *P);

    void emitSingleRQMemRayWrite(RTBuilder& builder, Value* queryObjIndex);
    bool analyzeSingleRQMemRayWrite(const Function& F) const;
    std::pair<bool, unsigned> analyzeSingleRQProceed(const Function& F) const;
    void HandleAcceptHitAndEndSearch(RTBuilder& builder, RTBuilder::SyncStackPointerVal* ShadowMemStackPointer, Instruction* IP);

    Value* emitProceedMainBody(RTBuilder& builder, Value* queryObjIndex);

    bool forceShortCurcuitingOR_CommittedGeomIdx(RTBuilder& builder, Instruction* I);
};

char TraceRayInlineLoweringPass::ID = 0;

// Register pass to igc-opt
#define PASS_FLAG "igc-tracerayinline-lowering-pass"
#define PASS_DESCRIPTION "Lower tracerayinline intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(TraceRayInlineLoweringPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(TraceRayInlineLoweringPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

bool TraceRayInlineLoweringPass::runOnFunction(Function& F)
{
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
    LI = &getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

    if (!m_CGCtx->platform.supportRayTracing())
        return false;

    singleRQMemRayStore = analyzeSingleRQMemRayWrite(F);
    unsigned numProceeds;
    std::tie(singleRQProceed, numProceeds) = analyzeSingleRQProceed(F);

    if (!LowerAllocateRayQuery(F, numProceeds))
        return false;

    LowerTraceRayInline(F);
    LowerTraceRaySyncProceedIntrinsic(F);
    LowerSyncStackToShadowMemory(F);
    LowerAbort(F);
    LowerCommittedStatus(F);
    LowerCandidateType(F);
    LowerRayInfo(F);
    LowerCommitNonOpaqueTriangleHit(F);
    LowerCommitProceduralPrimitiveHit(F);

    auto* modMD = m_CGCtx->getModuleMetaData();
    uint32_t numSyncRTStacks = m_CGCtx->syncRTCallsNeedSplitting() ? 2 : 1;

    modMD->rtInfo.numSyncRTStacks =
        std::max(modMD->rtInfo.numSyncRTStacks, numSyncRTStacks);

    auto FMD = modMD->FuncMD.find(&F);
    if (FMD != modMD->FuncMD.end()) {
      FMD->second.rtInfo.numSyncRTStacks = numSyncRTStacks;
    }

    DumpLLVMIR(m_CGCtx, "TraceRayInlineLoweringPass");
    return true;
}


bool TraceRayInlineLoweringPass::LowerAllocateRayQuery(
    Function& F, unsigned numProceeds)
{
    SmallVector<ConvertRayQueryHandleToRTStackPointerIntrinsic*> convertRayQueryToRTStackPointers;
    SmallVector<AllocateRayQueryIntrinsic*> AllocateRayQueries;
    for (auto& I : instructions(F))
    {
        if (auto* ARQ = dyn_cast<AllocateRayQueryIntrinsic>(&I))
            AllocateRayQueries.push_back(ARQ);
        else if (auto* II = dyn_cast<ConvertRayQueryHandleToRTStackPointerIntrinsic>(&I))
            convertRayQueryToRTStackPointers.push_back(II);
    }

    if (AllocateRayQueries.empty())
        return false;

    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();
    if (modMD->FuncMD.find(&F) == modMD->FuncMD.end()) {
        IGC::FunctionMetaData funcMd;
        funcMd.functionType = FunctionTypeMD::KernelFunction;
        modMD->FuncMD.insert(std::make_pair(&F, funcMd));
    }
    modMD->FuncMD[&F].hasSyncRTCalls = true;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);
    //let's use a very conservative way to shrink SharedMem size for now:
    //if we have more RQO (rayquery object)s than SIMD, we will have to use PTSS to hold them;
    //on the other hand, we DO need to make sure these RQOs won't overlap in which case we cannot shrink SharedMem like the current way
    //later, we might improve this w/ a more general way.
    //this way might cover quite some RQOs cases, though
    bool bShrinkSMStack = (AllocateRayQueries.size() > numLanes(m_CGCtx->platform.getMaxRayQuerySIMDSize(m_CGCtx->type))
        && numProceeds == 1 && convertRayQueryToRTStackPointers.empty());

    std::tie(m_ShMemRTStacks, m_ShMemRTCtrls) =
        builder.createAllocaRayQueryObjects(AllocateRayQueries.size(), bShrinkSMStack, VALUE_NAME("&ShadowMemory.RayQueryObjects"));

    unsigned int currentQueryIndex = 0;
    for (auto* ARQ : AllocateRayQueries)
    {
        builder.SetInsertPoint(ARQ);
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, currentQueryIndex);
        builder.setRayFlags(ShadowMemStackPointer, builder.CreateTrunc(ARQ->getFlags(), builder.getInt16Ty()));
        Value* currentIndex = builder.getInt32(currentQueryIndex++);
        ARQ->replaceAllUsesWith(currentIndex);
    }

    for (auto* ARQ : AllocateRayQueries)
    {
        ARQ->eraseFromParent();
    }

    for (auto* I : convertRayQueryToRTStackPointers)
    {
        builder.SetInsertPoint(I);
        auto* rtstack = getShMemRayQueryRTStack(builder, I->getQueryObjIndex());
        I->replaceAllUsesWith(rtstack);
        I->eraseFromParent();
    }

    return true;
}

std::pair<bool, unsigned>
TraceRayInlineLoweringPass::analyzeSingleRQProceed(const Function& F) const
{
    unsigned cntProceeds = 0;
    bool Result = true;
    for (auto& I : instructions(F))
    {
        if (auto *PI = dyn_cast<TraceRaySyncProceedIntrinsic>(&I)) {
            auto* RQO = dyn_cast<AllocateRayQueryIntrinsic>(PI->getQueryObjIndex());
            ++cntProceeds;
            Result &=
                (cntProceeds == 1) &&
                (!LI->getLoopFor(PI->getParent()) ||
                 (RQO && RQO->getParent() == PI->getParent()));
        }
    }

    // If STOC is enabled, it is possible to execute Proceed more than
    // once, despite the application does not call it in the loop.
    // In such case switch to multiple Proceeds mode.
    if (m_CGCtx->isSWSubTriangleOpacityCullingEmulationEnabled())
    {
        Result = false;
    }

    return { Result, cntProceeds };
}

//FIXME: temp solution, will use alias based general solution to replace this.
//this temp solution is more like a prototype/experiment to confirm if this way will improve performance enough
//which means we DO need a general solution eventually

//if
//  there's only one TraceRayInline() &&
//  zero or more Proceed() &&
//  TRI is not in loop &&
//  both intrinsics are for the same RQO
//then
//  we only need to write MemRay[TOP_LEVEL_BVH] data once.
bool TraceRayInlineLoweringPass::analyzeSingleRQMemRayWrite(const Function& F) const
{
    if (IGC_IS_FLAG_DISABLED(EnableSingleRQMemRayStore))
        return false;

    const Value* RQO = nullptr;
    const TraceRayInlineHLIntrinsic* TRI = nullptr;
    for (auto& I : instructions(F))
    {
        const Value* curRQO = nullptr;
        if (auto* tri = dyn_cast<TraceRayInlineHLIntrinsic>(&I))
        {
            if (TRI)
            {
                //we only work on single TRI case
                return false;
            }
            else
            {
                TRI = tri;
                curRQO = TRI->getQueryObjIndex();
            }
        }
        else if (auto* P = dyn_cast<TraceRaySyncProceedIntrinsic>(&I))
        {
            curRQO = P->getQueryObjIndex();
        }

        //make sure all RQOs are the same one (will replace logic w/ AA later)
        if (RQO && curRQO && (RQO != curRQO))
            return false;
        else if (!RQO && curRQO)
            RQO = curRQO;
    }

    //exclude case where TRI is in loop
    return RQO && TRI && !LI->getLoopFor(TRI->getParent());
}

void TraceRayInlineLoweringPass::LowerTraceRayInline(Function& F)
{
    vector<TraceRayInlineHLIntrinsic*> traceCalls;
    for (auto& I : instructions(F))
    {
        if (auto* TRI = dyn_cast<TraceRayInlineHLIntrinsic>(&I))
            traceCalls.push_back(TRI);
    }

    for (auto* trace : traceCalls)
    {
        RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);
        builder.SetInsertPoint(trace);
        Value* QueryObjIndex = trace->getQueryObjIndex();

        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, QueryObjIndex);
        {
            Value* Vec = UndefValue::get(
                IGCLLVM::FixedVectorType::get(
                    builder.getFloatTy(), trace->getNumRayInfoFields()));
            for (unsigned int i = 0; i < trace->getNumRayInfoFields(); i++)
                Vec = builder.CreateInsertElement(Vec, trace->getRayInfo(i), i);

            builder.createTraceRayInlinePrologue(
                ShadowMemStackPointer,
                Vec,
                builder.getRootNodePtr(trace->getBVH()),
                trace->getFlag(),
                trace->getMask(),
                trace->getComparisonValue(),
                trace->getTMax());
        }

        // Set TraceRayControl to Initial
        // RayQueryObject->stateInfo.traceRayCtrl = TRACE_RAY_INITIAL
        builder.setSyncTraceRayControl(
            getShMemRTCtrl(builder, QueryObjIndex),
            TraceRayCtrl::TRACE_RAY_INITIAL);

        if (singleRQMemRayStore)
        {
            emitSingleRQMemRayWrite(builder, QueryObjIndex);
        }
    }

    for (auto* trace : traceCalls)
    {
        trace->eraseFromParent();
    }
}

std::pair<BasicBlock*, BasicBlock*>
TraceRayInlineLoweringPass::branchOnPotentialHitDone(
    RTBuilder& IRB,
    RayQueryIntrinsicBase* P)
{
    auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(IRB, P->getQueryObjIndex());
    Value* NotDone = IRB.isDoneBitNotSet(ShadowMemStackPointer, false);

    return IRB.createTriangleFlow(
        NotDone, P, VALUE_NAME("ProceedBB"), VALUE_NAME("ProceedEndBlock"));
}

void TraceRayInlineLoweringPass::emitSingleRQMemRayWrite(RTBuilder& builder, Value* queryObjIndex)
{
    auto* const HWStackPointer = builder.getSyncStackPointer();
    auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, queryObjIndex);

    builder.emitSingleRQMemRayWrite(HWStackPointer, ShadowMemStackPointer, singleRQProceed);
}

Value* TraceRayInlineLoweringPass::emitProceedMainBody(
    RTBuilder& builder, Value* queryObjIndex)
{
    if (!singleRQMemRayStore)
    {
        emitSingleRQMemRayWrite(builder, queryObjIndex);
    }

    DenseMap<uint32_t, Value*> vals;

    auto* const HWStackPointer = builder.getSyncStackPointer();
    auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, queryObjIndex);

    builder.copyMemHitInProceed(HWStackPointer, ShadowMemStackPointer, singleRQProceed);

    //get ray Current ray control for object
    GetElementPtrInst* ShdowMemRTCtrlPtr = getShMemRTCtrl(builder, queryObjIndex);
    LoadInst* traceRayCtrl = builder.getSyncTraceRayControl(ShdowMemRTCtrlPtr);

    if (IGC_IS_FLAG_ENABLED(DisableLoadAsFenceOpInRaytracing))
    {
        builder.CreateLSCFence(LSC_UGM, LSC_SCOPE_LOCAL, LSC_FENCE_OP_NONE);
    }
    else
    {
        // this is an optimization
        // it's based on the idea that stores and loads are queued, so if a load completes, all stores before it are also completed
        // the requirement is that the load and the store should use the same address, so we use the potential hit (last write in copyMemHitInProceed)
        auto* potentialHit = builder.getHitAddress(HWStackPointer, false);

        auto* M = builder.GetInsertPoint()->getModule();
        auto* fn = GenISAIntrinsic::getDeclaration(
            M,
            GenISAIntrinsic::GenISA_LSCLoadWithSideEffects,
            { builder.getInt32Ty(), potentialHit->getType() }
        );

        builder.CreateCall(
            fn,
            {
                potentialHit,
                builder.getInt32(0),
                builder.getInt32(LSC_DATA_SIZE_32b),
                builder.getInt32(LSC_DATA_ELEMS_1),
                builder.getInt32(LSC_L1C_WT_L3C_WB)
            },
            VALUE_NAME("LSCLoadAsFence")
        );
    }


    //TraceRay
    Value* retSyncRT = builder.createSyncTraceRay(
        builder.getBvhLevel(ShadowMemStackPointer, false),
        traceRayCtrl,
        nullptr,
        VALUE_NAME("trace_ray_query"));

    return retSyncRT;
}

//Proceed Flow below is falling into 2 different intrinsics:
//LowerTraceRaySyncProceedIntrinsic(...){
//    //Abort if potentialHit.done is set
//    retSyncTR = false;
//    if (potentialHit.done)
//      return retSyncTR;

//    //we set potentialHit.done, which will get cleared by hardware for intersection and anyhit traversal
//    potentialHit.done = true;
//    //To continue tracing we have to spill/fill the HWMemory's sync rtStack back and forth from/to ShadowMemory's one
//    HWMemory.RTStack = ShadowMemory.RayQueryObject.RTStack;
//    createSyncTraceRay(); //Sync bit set to 1
//    return retSyncTR;
//}
//........
//LowerSyncStackToShadowMemory(int retSyncTR){
//    //Abort if potentialHit.done is set
//    if (potentialHit.done)
//      return false;
//    ReadSyncTraceRay(retSyncTR);
//    ShadowMemory.RayQueryObject.RTStack = HWMemory.RTStack
//    // Initially we use TRACE_RAY_INITIAL, but from now on we have to use TRACE_RAY_CONTINUE
//    obj.ctrl = TRACE_RAY_CONTINUE;
//    return !potentialHit.done;

void TraceRayInlineLoweringPass::LowerTraceRaySyncProceedIntrinsic(Function& F)
{
    vector<TraceRaySyncProceedIntrinsic*> proceeds;

    for (auto& I : instructions(F))
    {
        if (auto* P = dyn_cast<TraceRaySyncProceedIntrinsic>(&I))
            proceeds.push_back(P);
    }

    if (proceeds.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto *P : proceeds)
    {
        auto* StartBB = P->getParent();

        builder.SetInsertPoint(P);
        auto [ProceedBB, _] = branchOnPotentialHitDone(builder, P);

        auto* InsertPt = ProceedBB->getTerminator();
        builder.SetInsertPoint(InsertPt);
        Value* retProceed = emitProceedMainBody(builder, P->getQueryObjIndex());

        builder.SetInsertPoint(P);
        auto* phi = builder.CreatePHI(P->getType(), 2);
        phi->addIncoming(builder.CreateZExtOrTrunc(retProceed, phi->getType()), InsertPt->getParent());
        phi->addIncoming(builder.CreateZExtOrTrunc(builder.getFalse(), phi->getType()), StartBB);
        P->replaceAllUsesWith(phi);
    }

    for (auto P : proceeds)
    {
        P->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::LowerSyncStackToShadowMemory(Function& F)
{
    vector<RayQuerySyncStackToShadowMemory*> SS2SMs;

    for (auto& I : instructions(F))
    {
        if (auto* SS2SM = dyn_cast<RayQuerySyncStackToShadowMemory>(&I))
            SS2SMs.push_back(SS2SM);
    }

    if (SS2SMs.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto* SS2SM : SS2SMs)
    {
        builder.SetInsertPoint(SS2SM);

        Value* queryObjIndex = SS2SM->getQueryObjIndex();
        auto* const HWStackPointer = builder.getSyncStackPointer();
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, queryObjIndex);
        Value* ShadowMemRTCtrlPtr = getShMemRTCtrl(builder, queryObjIndex);

        auto* Proceed = builder.syncStackToShadowMemory(
            HWStackPointer,
            ShadowMemStackPointer,
            SS2SM->getProceedReturnVal(),
            ShadowMemRTCtrlPtr);

        SS2SM->replaceAllUsesWith(Proceed);

        if (IGC_IS_FLAG_DISABLED(DisableInvalidateRTStackAfterLastRead))
        {
            auto* fn = GenISAIntrinsic::getDeclaration(
                F.getParent(),
                GenISAIntrinsic::GenISA_LSCLoadWithSideEffects,
                { builder.getInt32Ty(), HWStackPointer->getType() }
            );

            auto [InvalidateAfterProceedBB, _] = builder.createTriangleFlow(builder.CreateNot(Proceed), SS2SM, VALUE_NAME("InvalidateAfterProceed"));
            builder.SetInsertPoint(InvalidateAfterProceedBB->getTerminator());

            LSC_L1_L3_CC CacheCtrl =
                m_CGCtx->platform.isSupportedLSCCacheControlsEnum(LSC_L1IAR_L3IAR, true) ?
                LSC_L1IAR_L3IAR :
                LSC_L1IAR_WB_L3C_WB;

            for (uint i = 0; i < getSyncStackSize() / m_CGCtx->platform.LSCCachelineSize(); i++)
            {
                builder.CreateCall(
                    fn,
                    {
                        HWStackPointer,
                        builder.getInt32(i * m_CGCtx->platform.LSCCachelineSize()),
                        builder.getInt32(LSC_DATA_SIZE_32b), // doesn't matter what we put here because the entire cacheline is invalidated
                        builder.getInt32(LSC_DATA_ELEMS_1),
                        builder.getInt32(CacheCtrl)
                    }
                );
            }
        }
    }

    for (auto SS2SM : SS2SMs)
    {
        SS2SM->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::LowerAbort(Function& F)
{
    vector<RayQueryAbortIntrinsic*> aborts;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryAbortIntrinsic>(&I))
            aborts.push_back(intrin);
    }

    if (aborts.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto abort : aborts)
    {
        builder.SetInsertPoint(abort);
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, abort->getQueryObjIndex());
        builder.setDoneBit(ShadowMemStackPointer, false);
    }

    for (auto abort : aborts)
    {
        abort->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::LowerCommittedStatus(Function& F)
{
    vector<RayQueryCommittedStatusIntrinsic*> CSes;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryCommittedStatusIntrinsic>(&I))
            CSes.push_back(intrin);
    }

    if (CSes.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto CS : CSes)
    {
        builder.SetInsertPoint(CS);

        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, CS->getQueryObjIndex());
        auto *Status = builder.getCommittedStatus(ShadowMemStackPointer);
        CS->replaceAllUsesWith(Status);
    }

    for (auto CS : CSes)
    {
        CS->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::LowerCandidateType(Function& F)
{
    vector<RayQueryCandidateTypeIntrinsic*> CTs;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryCandidateTypeIntrinsic>(&I))
            CTs.push_back(intrin);
    }

    if (CTs.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for(auto CT : CTs)
    {
        builder.SetInsertPoint(CT);
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, CT->getQueryObjIndex());
        auto* CandidateType = builder.getCandidateType(ShadowMemStackPointer);

        CT->replaceAllUsesWith(CandidateType);
    }

    for (auto CT : CTs)
    {
        CT->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::LowerRayInfo(Function& F)
{
    vector<RayQueryInfoIntrinsic*> info;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryInfoIntrinsic>(&I))
            info.push_back(intrin);
    }

    if (info.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto I : info)
    {
        builder.SetInsertPoint(I);

        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, I->getQueryObjIndex());

        switch (I->getInfoKind())
        {
        default:
            I->replaceAllUsesWith(
                builder.lowerRayInfo(ShadowMemStackPointer,
                    I,
                    builder.getInt32(I->isCommitted() ? ClosestHit : AnyHit),
                    std::nullopt
                )
            );
            I->eraseFromParent();
            break;
            // leave this in for now, until we prove we don't need the hack anymore
        case GEOMETRY_INDEX:
        {
            bool specialPattern = false;
            if (I->isCommitted() && IGC_GET_FLAG_VALUE(ForceRTShortCircuitingOR))
            {
                specialPattern = forceShortCurcuitingOR_CommittedGeomIdx(builder, I);
            }

            Value* leafType = builder.getLeafType(ShadowMemStackPointer, builder.getInt1(I->isCommitted()));
            Value* geoIndex = builder.getGeometryIndex(ShadowMemStackPointer, I, leafType,
                builder.getInt32(I->isCommitted() ? CallableShaderTypeMD::ClosestHit : CallableShaderTypeMD::AnyHit), !specialPattern);
            IGC_ASSERT_MESSAGE(I->getType()->isIntegerTy(), "Invalid geometryIndex type!");
            I->replaceAllUsesWith(geoIndex);
            I->eraseFromParent();
            break;
        }
        }
    }
}

void TraceRayInlineLoweringPass::LowerCommitNonOpaqueTriangleHit(Function& F)
{
    vector<RayQueryCommitNonOpaqueTriangleHit*> CommitHits;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryCommitNonOpaqueTriangleHit>(&I))
            CommitHits.push_back(intrin);
    }

    if (CommitHits.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto CH : CommitHits)
    {
        builder.SetInsertPoint(CH);
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, CH->getQueryObjIndex());

        builder.createPotentialHit2CommittedHit(ShadowMemStackPointer);
        builder.setSyncTraceRayControl(getShMemRTCtrl(builder, CH->getQueryObjIndex()), TraceRayCtrl::TRACE_RAY_COMMIT);

        HandleAcceptHitAndEndSearch(builder, ShadowMemStackPointer, CH);
    }

    for (auto CH : CommitHits)
    {
        CH->eraseFromParent();
    }
}

//NOTE: workload specific logic, don't use it for common case!
//only keep this logic here to make the HLK test pass before we get correct test
// change:
// if (a || (q.CommittedGeometryIndex() < q.CandidateGeometryIndex())
//  do_sth;
// to:
// if (a)
//     do_sth;
// else if (q.CommittedGeometryIndex() < q.CandidateGeometryIndex())
//     do_sth;
//------------IR----------------------
//old:==============================
//  %lhs = ...
//  % 47 = call i32 @llvm.genx.GenISA.TraceRayInlineRayInfo.i32(i32 % 13, i32 14, i32 0) //CommittedGeometryIndex()
//  % 48 = ...
//  % rhs = icmp ult i32 % 47, % 48
//  % orRes = or i1 % lhs, % rhs
//  br i1 % orRes, label % orBB, label % endBB
//
//  orBB:
//  call void ...
//
//  endBB:
//  call void ...
//
//new:==============================
//  %lhs = ...
//  br i1 % lhs, label % orBB, label % rhsBB
//
//  rhsBB:
//  % 47 = call i32 @llvm.genx.GenISA.TraceRayInlineRayInfo.i32(i32 % 13, i32 14, i32 0) //CommittedGeometryIndex()
//  % 48 = ...
//  % rhs = icmp ult i32 % 47, % 48
//  % orRes = or i1 % lhs, % rhs
//  br i1 % orRes, label % orBB, label % endBB
//  Note, above br still uses orRes to simplify the change. lhs == 0 here anyway
//
//  orBB:
//  call void ...
//
//  endBB:
//  call void ...
bool TraceRayInlineLoweringPass::forceShortCurcuitingOR_CommittedGeomIdx(RTBuilder& builder, Instruction* I)
{
    bool found = false;
    Instruction* lhs = nullptr;
    Instruction* rhs = nullptr;
    Instruction* orI = nullptr;
    BranchInst* brI = nullptr;
    for (auto U1 : I->users())
    {
        if (isa<ICmpInst>(U1))
        {   //found 2nd condition
            for (auto U2 : U1->users())
            {
                if ((orI = dyn_cast<Instruction>(U2)))
                {
                    if (orI->getOpcode() == Instruction::Or)
                    {
                        brI = dyn_cast<llvm::BranchInst>(*orI->user_begin());
                        lhs = dyn_cast<Instruction>(orI->getOperand(0));
                        rhs = dyn_cast<Instruction>(orI->getOperand(1));
                        found = (orI->getOperand(1) == U1 && brI && lhs && rhs);
                        if (found)
                        {
                            break;
                        }
                    }
                }
            }
        }
    }
    if (!found)
    {
        return false;
    }

    BasicBlock* orBB = brI->getSuccessor(0);

    auto* lhsBlock = lhs->getParent();
    auto* rhsBB = lhsBlock->splitBasicBlock(++lhs->getIterator(), VALUE_NAME("rhsBB"));
    lhsBlock->getTerminator()->eraseFromParent();
    builder.SetInsertPoint(lhsBlock);
    builder.CreateCondBr(lhs, orBB, rhsBB);

    builder.SetInsertPoint(I);

    //orI->eraseFromParent();
#if defined( _DEBUG )
    llvm::verifyModule(*m_CGCtx->getModule());
#endif

    return true;
}

void TraceRayInlineLoweringPass::LowerCommitProceduralPrimitiveHit(Function& F)
{
    vector<RayQueryCommitProceduralPrimitiveHit*> CommitHits;

    for (auto& I : instructions(F))
    {
        if (auto * intrin = dyn_cast<RayQueryCommitProceduralPrimitiveHit>(&I))
            CommitHits.push_back(intrin);
    }

    if (CommitHits.empty())
        return;

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);

    for (auto CH : CommitHits)
    {
        builder.SetInsertPoint(CH);
        auto* const ShadowMemStackPointer = getShMemRayQueryRTStack(builder, CH->getQueryObjIndex());
        builder.commitProceduralPrimitiveHit(ShadowMemStackPointer, CH->getTHit());

        builder.setSyncTraceRayControl(
            getShMemRTCtrl(builder, CH->getQueryObjIndex()),
            TraceRayCtrl::TRACE_RAY_COMMIT);

        HandleAcceptHitAndEndSearch(builder, ShadowMemStackPointer, CH);
    }

    for (auto CH : CommitHits)
    {
        CH->eraseFromParent();
    }
}

void TraceRayInlineLoweringPass::HandleAcceptHitAndEndSearch(RTBuilder& builder, RTBuilder::SyncStackPointerVal* ShadowMemStackPointer, Instruction* IP)
{
    auto* rayFlags = builder.getRayFlags(ShadowMemStackPointer);
    auto* AcceptHitAndEndSearch = builder.getInt16(static_cast<uint32_t>(RTStackFormat::RayFlags::ACCEPT_FIRST_HIT_AND_END_SEARCH));

    Value* V;

    V = builder.CreateAnd(rayFlags, AcceptHitAndEndSearch);
    V = builder.CreateICmpNE(V, builder.getInt16(0));

    auto [accepthitBB, _] = builder.createTriangleFlow(V, IP, VALUE_NAME("AcceptHitAndEndSearch"), VALUE_NAME("Default"));
    builder.SetInsertPoint(accepthitBB->getTerminator());
    builder.setDoneBit(ShadowMemStackPointer, false);
}

//For 3D/Compute Shaders The RTGlobals Pointer comes from a different location.
class RTGlobalsPointerLoweringPass : public FunctionPass
{
public:
    RTGlobalsPointerLoweringPass() : FunctionPass(ID) {}
    bool runOnFunction(Function& F) override;
    llvm::StringRef getPassName() const override
    {
        return "RTGlobalsPointerLoweringPass";
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }
    static char ID;

private:
    CodeGenContext* m_CGCtx = nullptr;
    static bool needsSplitting(const CodeGenContext* Ctx);
};

char RTGlobalsPointerLoweringPass::ID = 0;

bool RTGlobalsPointerLoweringPass::runOnFunction(Function& F)
{
    m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

    if (!m_CGCtx->platform.supportRayTracing())
        return false;

    vector<GenIntrinsicInst*> globalBuffPtrs;
    for (auto& I : instructions(F))
    {
        if (isa<GenIntrinsicInst>(&I, GenISAIntrinsic::GenISA_GlobalBufferPointer))
            globalBuffPtrs.push_back(cast<GenIntrinsicInst>(&I));
    }

    RTBuilder builder(&*F.getEntryBlock().begin(), *m_CGCtx);
    ModuleMetaData* modMD = m_CGCtx->getModuleMetaData();

    IGC_ASSERT_MESSAGE(nullptr != modMD,
        "Invalid Module Metadata in RTGlobalsPointerLoweringPass");

    const bool NeedsSplitting = m_CGCtx->syncRTCallsNeedSplitting();

    for (auto* GBP : globalBuffPtrs)
    {
        builder.SetInsertPoint(GBP);
        Function* pFunc =
            GenISAIntrinsic::getDeclaration(
                F.getParent(),
                GenISAIntrinsic::GenISA_RuntimeValue,
                GBP->getType());

        Value* rtGlobalsPtr = builder.CreateCall(
            pFunc,
            builder.getInt32(modMD->pushInfo.inlineRTGlobalPtrOffset));
        rtGlobalsPtr->takeName(GBP);

        Value* rtGlobalsPtrSplit = rtGlobalsPtr;
        if (NeedsSplitting)
        {
            uint32_t Addrspace =
                rtGlobalsPtr->getType()->getPointerAddressSpace();
            auto* LaneId = builder.get32BitLaneID();
            auto* Cond = builder.CreateICmpULT(
                LaneId, builder.getInt32(numLanes(SIMDMode::SIMD16)));
            auto* Ptr = builder.CreateBitCast(
                rtGlobalsPtr, builder.getInt8PtrTy(Addrspace));
            // UMD will allocate back-to-back RTGlobals if requested. The upper
            // 16 lanes will get the pointer to the second one.
            // We need at least 64-byte alignment. Let's just align both
            // structures to `RTGlobalsAlign`.
            constexpr uint32_t Offset =
                IGC::Align(sizeof(RayDispatchGlobalData), IGC::RTGlobalsAlign);
            Ptr = builder.CreateGEP(builder.getInt8Ty(), Ptr, builder.getInt32(Offset));
            auto* rtGlobalsPtrHi = builder.CreateBitCast(
                Ptr, rtGlobalsPtr->getType());
            rtGlobalsPtrSplit = builder.CreateSelect(
                Cond,
                rtGlobalsPtr,
                rtGlobalsPtrHi,
                VALUE_NAME("split.global.pointer"));
        }

        GBP->replaceAllUsesWith(rtGlobalsPtrSplit);
    }

    for (auto* GBP : globalBuffPtrs)
    {
        GBP->eraseFromParent();
    }

    return true;
}

namespace IGC
{
    Pass* CreateTraceRayInlineLoweringPass()
    {
        return new TraceRayInlineLoweringPass();
    }

    Pass* CreateRTGlobalsPointerLoweringPass()
    {
        return new RTGlobalsPointerLoweringPass();
    }
}
