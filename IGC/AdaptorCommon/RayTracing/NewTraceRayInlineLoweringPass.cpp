/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "IGC/common/StringMacros.hpp"
#include "IGC/common/LLVMUtils.h"
#include "NewTraceRayInlineLoweringPass.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/DenseMapInfo.h>
#include <llvm/ADT/Hashing.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "common/LLVMWarningsPop.hpp"

using namespace IGC;
using namespace llvm;

namespace llvm {
template <> struct DenseMapInfo<IGC::AllocationLivenessAnalyzer::LivenessData::Edge> {
  using Edge = IGC::AllocationLivenessAnalyzer::LivenessData::Edge;

  static inline Edge getEmptyKey() {
    return Edge{DenseMapInfo<BasicBlock *>::getEmptyKey(), DenseMapInfo<BasicBlock *>::getEmptyKey()};
  }

  static inline Edge getTombstoneKey() {
    return Edge{DenseMapInfo<BasicBlock *>::getTombstoneKey(), DenseMapInfo<BasicBlock *>::getTombstoneKey()};
  }

  static unsigned getHashValue(const Edge &E) { return (unsigned)hash_combine(E.from, E.to); }

  static bool isEqual(const Edge &LHS, const Edge &RHS) { return LHS == RHS; }
};
} // namespace llvm

void InlineRaytracing::getAdditionalAnalysisUsage(AnalysisUsage &AU) const { AU.addRequired<CodeGenContextWrapper>(); }

bool InlineRaytracing::LowerAllocations(Function &F) {
  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);
  StringRef name = "RQObjectType";
  /*
   * struct RQObjectType
   * {
   *     uint32_t slot;
   *     uint32_t rayQueryPackedData;
   * }
   */

  SmallVector<AllocateRayQueryIntrinsic *> AllocateRQInstructions;

  for (auto &I : instructions(F)) {
    if (isa<AllocateRayQueryIntrinsic>(&I))
      AllocateRQInstructions.push_back(cast<AllocateRayQueryIntrinsic>(&I));
  }

  // no rayquery allocations
  if (AllocateRQInstructions.empty())
    return false;

  auto *rtstackTy = IRB.getRTStack2PtrTy(false);

  if (!m_RQObjectType)
    m_RQObjectType = StructType::create(*m_pCGCtx->getLLVMContext(), {IRB.getInt32Ty(), IRB.getInt32Ty()}, name);

  auto *createRQObjectFnTy = FunctionType::get(m_RQObjectType->getPointerTo(), IRB.getInt32Ty(), false);
  auto *createRQObjectFn = m_Functions[CREATE_RQ_OBJECT] =
      Function::Create(createRQObjectFnTy, GlobalValue::PrivateLinkage, VALUE_NAME("createRQObject"), F.getParent());

  auto *getStackPointerFnTy = FunctionType::get(rtstackTy, m_RQObjectType->getPointerTo(), false);
  auto *getStackPointerFn = m_Functions[GET_STACK_POINTER_FROM_RQ_OBJECT] = Function::Create(
      getStackPointerFnTy, GlobalValue::PrivateLinkage, VALUE_NAME("getStackPointerFn"), F.getParent());

  auto *getRQHandleFromRQObjectFnTy =
      FunctionType::get(IRB.getInt32Ty(), {m_RQObjectType->getPointerTo(), IRB.getInt32Ty()}, false);
  auto *getRQHandleFromRQObjectFn = m_Functions[GET_RQ_HANDLE_FROM_RQ_OJECT] = Function::Create(
      getRQHandleFromRQObjectFnTy, GlobalValue::PrivateLinkage, VALUE_NAME("getRQHandleFromRQObjectFn"), F.getParent());

  getStackPointerFn->addParamAttr(0, llvm::Attribute::NoCapture);

  // allocate rayquery instructions return i32 handle
  // we want all rayqueries to be represent via our struct
  // to do that, we allocate the object on the stack via alloca
  // then update all the users to either use the objects directly or to convert
  // them via our stub instruction
  ValueToValueMapTy v2vMap;
  for (auto *I : AllocateRQInstructions) {
    // we are using value remapper so we can't explicitly erase the instruction
    // mark it as read only so it gets removed due to no uses
    cast<CallInst>(I)->addFnAttr(llvm::Attribute::ReadOnly);

    IRB.SetInsertPoint(F.getEntryBlock().getFirstNonPHI());
    Value *rqObject = nullptr;

    IRB.SetInsertPoint(I);
    if (m_pCGCtx->syncRTCallsNeedSplitting()) {
      // create 2 rq objects and select one based on the lane id

      auto *rqObject1 = IRB.CreateCall(createRQObjectFn, UndefValue::get(IRB.getInt32Ty()), VALUE_NAME("RQObject"));
      auto *rqObject2 = IRB.CreateCall(createRQObjectFn, UndefValue::get(IRB.getInt32Ty()), VALUE_NAME("RQObject"));

      auto *laneId = IRB.get32BitLaneID();
      auto *cond = IRB.CreateICmpULT(laneId, IRB.getInt32(numLanes(SIMDMode::SIMD16)));
      rqObject = IRB.CreateSelect(cond, rqObject1, rqObject2, VALUE_NAME("RQObject"));
    } else {
      rqObject = IRB.CreateCall(createRQObjectFn, UndefValue::get(IRB.getInt32Ty()), VALUE_NAME("RQObject"));
    }

    // iniitalize it to done so when app calls proceed without tracerayinline,
    // we dont traverse over garbage
    setPackedData(
        IRB, rqObject,
        {IRB.getInt32(TRACE_RAY_DONE), IRB.getInt32(0), IRB.getInt32(0), IRB.getInt32(0), IRB.getInt32(CommittedHit)});

    v2vMap[I] = rqObject;

    SmallVector<Use *> worklist;

    for (auto &U : I->uses())
      worklist.push_back(&U);

    while (!worklist.empty()) {
      auto *use = worklist.pop_back_val();
      auto *II = cast<Instruction>(use->getUser());
      if (v2vMap.count(II) == 1)
        continue;

      if (auto *genI = dyn_cast<GenIntrinsicInst>(II)) {
        // clang-format off
        // for example, we have something like this:
        // %result = call fast float @llvm.genx.GenISA.TraceRayInlineRayInfo.f32(i32 %rayqueryhandle, i32 5, i32 0, i1 true)
        // what we want is:
        // %rayqueryhandle = call i32 @getRQHandleFromRQObjectFn(ptr %rayqueryobject, i32 %flags)
        // %result = call fast float @llvm.genx.GenISA.TraceRayInlineRayInfo.f32(i32 %rayqueryhandle, i32 5, i32 0, i1 true)
        // later, when lowering TraceRayInlineRayInfo,
        // we will just look up the operands of %rayqueryhandle and use them to lower the intrinsic
        // clang-format on

        IGC_ASSERT(v2vMap.count(use->get()) == 1);

        IRB.SetInsertPoint(genI);
        auto *RQHandle =
            IRB.CreateCall(getRQHandleFromRQObjectFn, {v2vMap[use->get()], I->getFlags()}, VALUE_NAME("RQHandle"));
        use->set(RQHandle);

        v2vMap[genI] = genI;
      } else {
        // in general case this would be very hard
        // fortunately rayqueries are guaranteed to not be stored in anything
        // more complex than a single dimensional array
        switch (II->getOpcode()) {
        case Instruction::Store: {
          auto *storeI = cast<StoreInst>(II);
          if (storeI->getValueOperand() == use->get() && v2vMap.count(storeI->getPointerOperand()) == 0) {
            SmallVector<Instruction *> origins;
            [[maybe_unused]] auto hasOrigins = Provenance::tryFindPointerOrigin(storeI->getPointerOperand(), origins);

            IGC_ASSERT_MESSAGE(hasOrigins, "Origin not found?");

            for (auto *origin : origins) {
              auto *array = cast<AllocaInst>(origin);
              if (v2vMap.count(origin) == 1)
                continue;

              auto *ty = ArrayType::get(m_RQObjectType->getPointerTo(),
                                        cast<ArrayType>(array->getAllocatedType())->getNumElements());

              IRB.SetInsertPoint(array);
              auto *newArray = IRB.CreateAlloca(ty, nullptr, VALUE_NAME("RQObjectArrayAlloca_") + array->getName(),
                                                array->getAddressSpace());
              v2vMap[array] = newArray;

              llvm::for_each(array->uses(), [&worklist](Use &U) { worklist.push_back(&U); });
            }
          } else {
            // skip if we didn't map out both operands yet
            if (v2vMap.count(II->getOperand(0)) == 0 || v2vMap.count(II->getOperand(1)) == 0)
              continue;

            // we are descending from the parent array, create a new store
            // instruction
            IRB.SetInsertPoint(II);
            v2vMap[II] = IRB.CreateStore(v2vMap[II->getOperand(0)], v2vMap[II->getOperand(1)]);
          }
        } break;
        case Instruction::GetElementPtr: {
          // we should be coming down from an array in this case
          IGC_ASSERT(v2vMap.count(II->getOperand(0)) == 1);
          auto *array = cast<AllocaInst>(v2vMap[II->getOperand(0)]);
          SmallVector<Value *> indices(cast<GetElementPtrInst>(II)->indices());

          IRB.SetInsertPoint(II);
          v2vMap[II] = IRB.CreateInBoundsGEP(array->getAllocatedType(), array, indices,
                                             VALUE_NAME("RQObjectGEP_") + II->getName());
          llvm::for_each(II->uses(), [&worklist](Use &U) { worklist.push_back(&U); });
        } break;
        case Instruction::Load:
          IRB.SetInsertPoint(II);
          v2vMap[II] = IRB.CreateLoad(m_RQObjectType->getPointerTo(), v2vMap[II->getOperand(0)],
                                      VALUE_NAME("RQObjectLoad_") + II->getName());
          llvm::for_each(II->uses(), [&worklist](Use &U) { worklist.push_back(&U); });
          break;
        case Instruction::Select:
          // skip if we didn't map out both operands yet
          if (v2vMap.count(II->getOperand(1)) == 0 || v2vMap.count(II->getOperand(2)) == 0)
            continue;

          IRB.SetInsertPoint(II);
          v2vMap[II] = IRB.CreateSelect(II->getOperand(0), v2vMap[II->getOperand(1)], v2vMap[II->getOperand(2)],
                                        VALUE_NAME("RQObjectSelect_") + II->getName());
          llvm::for_each(II->uses(), [&worklist](Use &U) { worklist.push_back(&U); });
          break;
        default:
          IGC_ASSERT(0);
          break;
        }
      }
    }
  }

  RemapFunction(F, v2vMap, RF_IgnoreMissingLocals | RF_ReuseAndMutateDistinctMDs);

  DenseSet<Instruction *> canBeDeleted;

  // try to remove as many of unused instructions as possible
  for (auto [from, _] : v2vMap) {

    if (auto *I = dyn_cast<Instruction>(const_cast<Value *>(from))) {

      if (I->getType()->isVoidTy())
        continue;

      if (isa<CallBase>(I))
        continue;

      canBeDeleted.insert(I);
    }
  }

  while (!canBeDeleted.empty()) {

    bool changed = false;
    for (auto *V : canBeDeleted) {

      if (V->use_empty()) {

        canBeDeleted.erase(V);
        cast<Instruction>(V)->eraseFromParent();
        changed = true;
        break;
      }
    }
    if (!changed) // no progress has been done
      break;
  }

  return true;
}

// NOTE: workload specific logic, don't use it for common case!
// only keep this logic here to make the HLK test pass before we get correct
// test
//  change:
//  if (a || (q.CommittedGeometryIndex() < q.CandidateGeometryIndex())
//   do_sth;
//  to:
//  if (a)
//      do_sth;
//  else if (q.CommittedGeometryIndex() < q.CandidateGeometryIndex())
//      do_sth;
//------------IR----------------------
// old:==============================
//   %lhs = ...
//   % 47 = call i32 @llvm.genx.GenISA.TraceRayInlineRayInfo.i32(i32 % 13, i32
//   14, i32 0) //CommittedGeometryIndex() % 48 = ... % rhs = icmp ult i32 % 47,
//   % 48 % orRes = or i1 % lhs, % rhs br i1 % orRes, label % orBB, label %
//   endBB
//
//   orBB:
//   call void ...
//
//   endBB:
//   call void ...
//
// new:==============================
//   %lhs = ...
//   br i1 % lhs, label % orBB, label % rhsBB
//
//   rhsBB:
//   % 47 = call i32 @llvm.genx.GenISA.TraceRayInlineRayInfo.i32(i32 % 13, i32
//   14, i32 0) //CommittedGeometryIndex() % 48 = ... % rhs = icmp ult i32 % 47,
//   % 48 % orRes = or i1 % lhs, % rhs br i1 % orRes, label % orBB, label %
//   endBB Note, above br still uses orRes to simplify the change. lhs == 0 here
//   anyway
//
//   orBB:
//   call void ...
//
//   endBB:
//   call void ...
static bool forceShortCurcuitingOR_CommittedGeomIdx(RTBuilder &builder, Instruction *I) {
  bool found = false;
  Instruction *lhs = nullptr;
  Instruction *rhs = nullptr;
  Instruction *orI = nullptr;
  BranchInst *brI = nullptr;
  for (auto U1 : I->users()) {
    if (isa<ICmpInst>(U1)) { // found 2nd condition
      for (auto U2 : U1->users()) {
        if ((orI = dyn_cast<Instruction>(U2))) {
          if (orI->getOpcode() == Instruction::Or) {
            brI = dyn_cast<llvm::BranchInst>(*orI->user_begin());
            lhs = dyn_cast<Instruction>(orI->getOperand(0));
            rhs = dyn_cast<Instruction>(orI->getOperand(1));
            found = (orI->getOperand(1) == U1 && brI && lhs && rhs);
            if (found) {
              break;
            }
          }
        }
      }
    }
  }
  if (!found) {
    return false;
  }

  BasicBlock *orBB = brI->getSuccessor(0);

  auto *lhsBlock = lhs->getParent();
  auto *rhsBB = lhsBlock->splitBasicBlock(++lhs->getIterator(), VALUE_NAME("rhsBB"));
  lhsBlock->getTerminator()->eraseFromParent();
  builder.SetInsertPoint(lhsBlock);
  builder.CreateCondBr(lhs, orBB, rhsBB);

  builder.SetInsertPoint(I);

  // orI->eraseFromParent();
#if defined(_DEBUG)
  llvm::verifyModule(*I->getFunction()->getParent());
#endif

  return true;
}

void InlineRaytracing::EmitPreTraceRayFence(RTBuilder &IRB, Value *rqObject) {
  {
    if (IGC_IS_FLAG_ENABLED(DisableLoadAsFenceOpInRaytracing)) {
      IRB.CreateLSCFence(LSC_UGM, LSC_SCOPE_LOCAL, LSC_FENCE_OP_NONE);
    } else {
      // this is an optimization
      // it's based on the idea that stores and loads are queued, so if a load
      // completes, all stores before it are also completed the requirement is
      // that the load and the store should use the same address, so we use the
      // potential hit (last write in copyMemHitInProceed)
      auto *potentialHit = IRB.getHitAddress(getStackPtr(IRB, rqObject), false);

      auto *M = IRB.GetInsertPoint()->getModule();
      auto *fn = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_LSCLoadWithSideEffects,
                                                 {IRB.getInt32Ty(), potentialHit->getType()});

      IRB.CreateCall(fn,
                     {potentialHit, IRB.getInt32(0), IRB.getInt32(LSC_DATA_SIZE_32b), IRB.getInt32(LSC_DATA_ELEMS_1),
                      IRB.getInt32(LSC_L1C_WT_L3C_WB)},
                     VALUE_NAME("LSCLoadAsFence"));
    }
  }
}


void InlineRaytracing::LowerIntrinsics(Function &F) {
  SmallVector<RayQueryIntrinsicBase *> RQInstructions;
  SmallVector<RayQueryInfoIntrinsic *> RQInfoInstructions;

  for (auto &I : instructions(F)) {
    if (isa<RayQueryIntrinsicBase>(&I))
      RQInstructions.push_back(cast<RayQueryIntrinsicBase>(&I));
  }

  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);

  for (auto RQI : RQInstructions) {
    auto *convertRQHandleFromRQObject = cast<Instruction>(RQI->getQueryObjIndex());
    auto *rqObject = convertRQHandleFromRQObject->getOperand(0);
    auto *rqFlags = convertRQHandleFromRQObject->getOperand(1);
    IRB.SetInsertPoint(RQI);

    switch (RQI->getIntrinsicID()) {
    case GenISAIntrinsic::GenISA_ConvertRayQueryHandleToRTStackPointer:
      RQI->replaceAllUsesWith(getStackPtr(IRB, rqObject));
      break;
    case GenISAIntrinsic::GenISA_TraceRayInlineHL: {
      auto *I = cast<TraceRayInlineHLIntrinsic>(RQI);
      Value *Vec = UndefValue::get(IGCLLVM::FixedVectorType::get(IRB.getFloatTy(), I->getNumRayInfoFields()));
      for (unsigned int i = 0; i < I->getNumRayInfoFields(); i++)
        Vec = IRB.CreateInsertElement(Vec, I->getRayInfo(i), i);

      auto *flags = IRB.CreateOr(I->getFlag(), rqFlags);

      IRB.createTraceRayInlinePrologue(getStackPtr(IRB, rqObject), Vec, IRB.getRootNodePtr(I->getBVH()), flags,
                                       I->getMask(), I->getComparisonValue(), I->getTMax(), false, true);

      auto *hasAcceptHitAndEndSearchFlag =
          IRB.CreateAnd(flags, static_cast<uint32_t>(RTStackFormat::RayFlags::ACCEPT_FIRST_HIT_AND_END_SEARCH));

      UnpackedData data;
      data.TraceRayCtrl = IRB.getInt32(TRACE_RAY_INITIAL);
      data.CommittedStatus = IRB.getInt32(0);
      data.CandidateType = IRB.getInt32(0);
      data.HasAcceptHitAndEndSearchFlag =
          IRB.CreateZExt(IRB.CreateICmpNE(hasAcceptHitAndEndSearchFlag, IRB.getInt32(0)), IRB.getInt32Ty());

      data.CommittedDataLocation = IRB.getInt32(CommittedHit);

      setPackedData(IRB, rqObject, data);

      // for the cross-block optimization purposes, split basic block to avoid using stale shadow stack
      if (allowCrossBlockLoadVectorization())
        IRB.createTriangleFlow(IRB.getFalse(), RQI);

      break;
    }
    case GenISAIntrinsic::GenISA_TraceRaySyncProceedHL: {
      auto data = getPackedData(IRB, rqObject);
      auto *traceRayCtrl = data.TraceRayCtrl;
      auto *doNotAbort = IRB.CreateICmpNE(traceRayCtrl, IRB.getInt32(TRACE_RAY_DONE));
      auto *result = IRB.CreatePHI(IRB.getInt1Ty(), 2, VALUE_NAME("ProceedResult"));
      result->addIncoming(IRB.getFalse(), result->getParent());
      auto [proceedBB, abortBB] =
          IRB.createTriangleFlow(doNotAbort, result, VALUE_NAME("NotAbortedProceedBB"), VALUE_NAME("PostProceedBB"));

      auto *entryBB = proceedBB->getUniquePredecessor();
      entryBB->getTerminator()->eraseFromParent();

      // clang-format off
      // there are 4 cases here:
      // 1. the ray was just initialized:
      //    enter the traversal block
      // 2. we are mid traversal and app did not commit any hit since last
      // proceed
      //    set the done bit to 1
      //    enter the traversal block
      // 3. we are mid traversal and app has committed a hit since last proceed
      //    set the done bit to 1
      //    set the valid bit to 1
      // 4. we are done with traversal
      //    skip the traversal block
      // clang-format on

      // Create a block to handle 2 and 3
      auto *setDoneBB = BasicBlock::Create(*m_pCGCtx->getLLVMContext(), VALUE_NAME("setDoneBB"), &F, proceedBB);

      IRB.SetInsertPoint(entryBB);
      auto *switchI = IRB.CreateSwitch(traceRayCtrl, setDoneBB, 2);
      switchI->addCase(IRB.getInt32(TRACE_RAY_DONE), abortBB);
      switchI->addCase(IRB.getInt32(TRACE_RAY_INITIAL), proceedBB);

      // add unreachable to the new block so we can split it
      IRB.SetInsertPoint(setDoneBB);
      auto *IP = IRB.CreateUnreachable();
      IRB.SetInsertPoint(IP);

      {
        // make sure the done bit is set to 0
        // if we start the traversal and the done bit is set to 1, HW will just
        // return
        // we conditionally set valid hit to 1 as well, see comment in
        // CommitProceduralPrimitiveHit case
        auto *cond = IRB.CreateICmpEQ(data.CommittedDataLocation, IRB.getInt32(CommittedDataLocation::PotentialHit));

        Instruction *ifTerm, *elseTerm;

        SplitBlockAndInsertIfThenElse(cond, IP, &ifTerm, &elseTerm);
        IRB.SetInsertPoint(ifTerm);
        IRB.setDoneBit(getStackPtr(IRB, rqObject), false);
        IRB.setHitValid(getStackPtr(IRB, rqObject), false);
        IRB.CreateBr(proceedBB);
        ifTerm->eraseFromParent();

        IRB.SetInsertPoint(elseTerm);
        IRB.setDoneBit(getStackPtr(IRB, rqObject), false);
        IRB.CreateBr(proceedBB);
        elseTerm->eraseFromParent();
      }

      IRB.SetInsertPoint(proceedBB->getFirstNonPHI());
      auto *bvhLevel = IRB.CreatePHI(IRB.getInt32Ty(), 2, VALUE_NAME("BVHLevel"));

      EmitPreTraceRayFence(IRB, rqObject);


      auto *globalBufferPtr = getGlobalBufferPtr(IRB, rqObject);
      CallInst *traceRay = IRB.createSyncTraceRay(bvhLevel, traceRayCtrl, globalBufferPtr);

      // add this for liveness analysis
      traceRay->addParamAttr(0, llvm::Attribute::NoCapture);

      IRB.createReadSyncTraceRay(traceRay);

      if (m_pCGCtx->platform.isRayQueryReturnOptimizationEnabled()) {

        // unpack the return value following the
        // RTStackFormat::RayQueryReturnData layout
        auto *proceedFurther = IRB.CreateAnd(
            traceRay, (1 << static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::proceed_further)) - 1);

        auto *committedStatus =
            IRB.CreateLShr(traceRay, static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::proceed_further));
        committedStatus =
            IRB.CreateAnd(committedStatus,
                          (1 << static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::committedStatus)) - 1);

        auto *candidateType = IRB.CreateLShr(
            traceRay, static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::proceed_further) +
                          static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::committedStatus));
        candidateType = IRB.CreateAnd(
            candidateType, (1 << static_cast<uint32_t>(RTStackFormat::RayQueryReturnData::Bits::candidateType)) - 1);

        auto *notDone = IRB.CreateICmpEQ(proceedFurther, IRB.getInt32(1));
        result->addIncoming(notDone, IRB.GetInsertBlock());

        data.TraceRayCtrl = IRB.CreateSelect(notDone, IRB.getInt32(TRACE_RAY_CONTINUE), IRB.getInt32(TRACE_RAY_DONE));

        data.CommittedStatus = committedStatus;
        data.CandidateType = candidateType;
        data.CommittedDataLocation = IRB.getInt32(CommittedHit);
      } else {
        auto *notDone = IRB.isDoneBitNotSet(getStackPtr(IRB, rqObject), false);
        result->addIncoming(notDone, IRB.GetInsertBlock());

        data.TraceRayCtrl = IRB.CreateSelect(notDone, IRB.getInt32(TRACE_RAY_CONTINUE), IRB.getInt32(TRACE_RAY_DONE));

        // we could technically defer loading these and check for RQ return
        // optimization later, but the gains from this are questionable (if app
        // isnt going to check these than theres no point to doing rayquery) and
        // doing this now lets us localize the branching that stems from RQ
        // return optimization

        data.CommittedStatus = IRB.getCommittedStatus(getStackPtr(IRB, rqObject));
        data.CandidateType = IRB.getCandidateType(getStackPtr(IRB, rqObject));
        data.CommittedDataLocation = IRB.getInt32(CommittedHit);
      }

      setPackedData(IRB, rqObject, data);

      for (auto *predBB : predecessors(proceedBB))
        bvhLevel->addIncoming(IRB.getInt32(predBB == switchI->getParent() ? RTStackFormat::TOP_LEVEL_BVH
                                                                          : RTStackFormat::BOTTOM_LEVEL_BVH),
                              predBB);
      // for the cross-block optimization purposes, split basic block to avoid using stale shadow stack
      if (allowCrossBlockLoadVectorization())
        IRB.createTriangleFlow(IRB.getFalse(), RQI);

      RQI->replaceAllUsesWith(result);
      break;
    }
    case GenISAIntrinsic::GenISA_TraceRaySyncProceed:
      IGC_ASSERT_MESSAGE(0, "Shouldn't be emitted!");
      break;
    case GenISAIntrinsic::GenISA_ShadowMemoryToSyncStack:
      IGC_ASSERT_MESSAGE(0, "Shouldn't be emitted!");
      break;
    case GenISAIntrinsic::GenISA_SyncStackToShadowMemory:
      IGC_ASSERT_MESSAGE(0, "Shouldn't be emitted!");
      break;
    case GenISAIntrinsic::GenISA_TraceRayInlineAbort: {
      auto data = getPackedData(IRB, rqObject);
      data.TraceRayCtrl = IRB.getInt32(TRACE_RAY_DONE);
      setPackedData(IRB, rqObject, data);
      break;
    }
    case GenISAIntrinsic::GenISA_TraceRayInlineCommittedStatus:
      RQI->replaceAllUsesWith(getPackedData(IRB, rqObject).CommittedStatus);
      break;
    case GenISAIntrinsic::GenISA_TraceRayInlineCandidateType:
      RQI->replaceAllUsesWith(getPackedData(IRB, rqObject).CandidateType);
      break;
    case GenISAIntrinsic::GenISA_TraceRayInlineRayInfo:
      RQInfoInstructions.push_back(cast<RayQueryInfoIntrinsic>(RQI));
      break;
    case GenISAIntrinsic::GenISA_TraceRayInlineCommitNonOpaqueTriangleHit: {
      auto data = getPackedData(IRB, rqObject);
      auto *notDone = IRB.CreateAnd({IRB.CreateICmpEQ(data.HasAcceptHitAndEndSearchFlag, IRB.getInt32(0)),
                                     IRB.CreateICmpNE(data.TraceRayCtrl, IRB.getInt32(TRACE_RAY_DONE))});

      data.CommittedDataLocation = IRB.getInt32(PotentialHit);
      data.TraceRayCtrl = IRB.CreateSelect(notDone, IRB.getInt32(TRACE_RAY_COMMIT), IRB.getInt32(TRACE_RAY_DONE));
      data.CommittedStatus = IRB.getInt32(RTStackFormat::COMMITTED_STATUS::COMMITTED_TRIANGLE_HIT);

      setPackedData(IRB, rqObject, data);
      break;
    }
    case GenISAIntrinsic::GenISA_TraceRayInlineCommitProceduralPrimitiveHit: {
      auto data = getPackedData(IRB, rqObject);
      auto *notDone = IRB.CreateAnd({IRB.CreateICmpEQ(data.HasAcceptHitAndEndSearchFlag, IRB.getInt32(0)),
                                     IRB.CreateICmpNE(data.TraceRayCtrl, IRB.getInt32(TRACE_RAY_DONE))});

      IRB.setHitT(getStackPtr(IRB, rqObject), cast<RayQueryCommitProceduralPrimitiveHit>(RQI)->getTHit(), false);

      // here we should set hit.valid to 1
      // however, this would emit RMW sequence and thus is not optimal for
      // performance we have to touch the same DWORD on Proceed anyway (to set
      // the hit.done to 0) so we will set hit.valid = 1 and hit.done = 1 in one
      // write when doing Proceed

      data.CommittedDataLocation = IRB.getInt32(PotentialHit);
      data.TraceRayCtrl = IRB.CreateSelect(notDone, IRB.getInt32(TRACE_RAY_COMMIT), IRB.getInt32(TRACE_RAY_DONE));
      data.CommittedStatus = IRB.getInt32(RTStackFormat::COMMITTED_STATUS::COMMITTED_PROCEDURAL_PRIMITIVE_HIT);

      setPackedData(IRB, rqObject, data);

      // for the cross-block optimization purposes, split basic block to avoid using stale shadow stack
      if (allowCrossBlockLoadVectorization())
        IRB.createTriangleFlow(IRB.getFalse(), RQI);

      break;
    }
    default:
      IGC_ASSERT_MESSAGE(0, "Missed an intrinsic?");
      break;
    }
  }

  // first map every rayinfo instruction to a stack pointer
  // we do it this way because rayinfo lowering itself will produce blocks
  // so a 2-pass method will yield better results
  MapVector<RayQueryInfoIntrinsic *, RTBuilder::SyncStackPointerVal *> RQInfoStackMap;

  for (auto *I : RQInfoInstructions) {

    auto *convertRQHandleFromRQObject = cast<Instruction>(I->getQueryObjIndex());
    auto *rqObject = convertRQHandleFromRQObject->getOperand(0);
    IRB.SetInsertPoint(I);
    RQInfoStackMap.insert(std::make_pair(I, getStackPtr(IRB, rqObject, true)));
  }

  // now we can actually lower rayinfo instructions
  for (const auto &[I, stackPtr] : RQInfoStackMap) {

    IRB.SetInsertPoint(I);
    auto *convertRQHandleFromRQObject = cast<Instruction>(I->getQueryObjIndex());
    auto *rqObject = convertRQHandleFromRQObject->getOperand(0);
    auto data = getPackedData(IRB, rqObject);
    auto *loadCommittedFromPotential = IRB.CreateICmpEQ(data.CommittedDataLocation, IRB.getInt32(PotentialHit),
                                                        VALUE_NAME("loadCommittedInfoFromPotentialHit"));

    auto *shaderTy = IRB.CreateSelect(loadCommittedFromPotential, IRB.getInt32(AnyHit),
                                      IRB.getInt32(I->isCommitted() ? ClosestHit : AnyHit));

    switch (I->getInfoKind()) {
    default:
      I->replaceAllUsesWith(IRB.lowerRayInfo(stackPtr, I, shaderTy, std::nullopt));
      break;
      // leave this in for now, until we prove we don't need the hack anymore
    case GEOMETRY_INDEX: {
      bool specialPattern = false;
      if (I->isCommitted() && IGC_GET_FLAG_VALUE(ForceRTShortCircuitingOR)) {
        specialPattern = forceShortCurcuitingOR_CommittedGeomIdx(IRB, I);
      }

      Value *leafType = IRB.getLeafType(stackPtr, IRB.getInt1(I->isCommitted()));
      Value *geoIndex = IRB.getGeometryIndex(
          stackPtr, I, leafType,
          IRB.getInt32(I->isCommitted() ? CallableShaderTypeMD::ClosestHit : CallableShaderTypeMD::AnyHit),
          !specialPattern);
      IGC_ASSERT_MESSAGE(I->getType()->isIntegerTy(), "Invalid geometryIndex type!");
      I->replaceAllUsesWith(geoIndex);
      break;
    }
    }
  }

  llvm::for_each(RQInstructions, [](RayQueryIntrinsicBase *I) {
    auto *RQHandle = cast<Instruction>(I->getQueryObjIndex());
    I->eraseFromParent();
    RQHandle->eraseFromParent();
  });
}

InlineRaytracing::LivenessDataMap InlineRaytracing::AnalyzeLiveness(Function &F, DominatorTree &DT, LoopInfo &LI) {
  LivenessDataMap data;
  for (auto *I : m_Functions[CREATE_RQ_OBJECT]->users()) {
    data.insert(std::make_pair(cast<Instruction>(I), ProcessInstruction(cast<Instruction>(I), DT, LI)));
  }

  return data;
}

void InlineRaytracing::AssignSlots(Function &F, const LivenessDataMap &livenessDataMap) {
  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);
  SmallVector<SmallVector<const LivenessData *>, 2> occupancyMap;

  if (IGC_IS_FLAG_SET(AddDummySlotsForNewInlineRaytracing)) {
    for (uint32_t i = 0; i < IGC_GET_FLAG_VALUE(AddDummySlotsForNewInlineRaytracing); i++) {
      occupancyMap.push_back(SmallVector<const LivenessData *>());
    }
  }

  for (auto &entry : livenessDataMap) {
    auto *I = entry.first;
    auto *LD = &entry.second;
    // <= in "slot <= occupancyMap.size()" is on purpose
    // this way, if we can't use any existing slot
    // we will construct occupancyMap[occupancyMap.size()] and use the newly
    // constructed slot
    for (uint32_t slot = IGC_GET_FLAG_VALUE(AddDummySlotsForNewInlineRaytracing); slot <= occupancyMap.size(); slot++) {
      bool newSlotNeeded = slot == occupancyMap.size();
      bool hasOverlaps = !newSlotNeeded && any_of(occupancyMap[slot], [&LD](const LivenessData *occupyingLD) {
        return occupyingLD->OverlapsWith(*LD);
      });

      if (hasOverlaps)
        continue;

      I->setOperand(0, IRB.getInt32(slot));

      if (newSlotNeeded)
        occupancyMap.push_back(SmallVector<const LivenessData *>());

      occupancyMap[slot].push_back(LD);

      break;
    }
  }

  m_numSlotsUsed = occupancyMap.size();
  IGC_ASSERT_MESSAGE(m_numSlotsUsed, "what??");
}

void InlineRaytracing::InsertCacheControl(RTBuilder &IRB, RTBuilder::SyncStackPointerVal *stackPtr) {
    if (IGC_IS_FLAG_DISABLED(DisableInvalidateRTStackAfterLastRead)) {
      auto *fn = GenISAIntrinsic::getDeclaration(m_pCGCtx->getModule(), GenISAIntrinsic::GenISA_LSCLoadWithSideEffects,
                                                 {IRB.getInt32Ty(), stackPtr->getType()});

      LSC_L1_L3_CC CacheCtrl = m_pCGCtx->platform.isSupportedLSCCacheControlsEnum(LSC_L1IAR_L3IAR, true)
                                   ? LSC_L1IAR_L3IAR
                                   : LSC_L1IAR_WB_L3C_WB;

      for (uint i = 0; i < IRB.getSyncStackSize() / m_pCGCtx->platform.LSCCachelineSize(); i++) {
        IRB.CreateCall(fn, {stackPtr, IRB.getInt32(i * m_pCGCtx->platform.LSCCachelineSize()),
                            IRB.getInt32(LSC_DATA_SIZE_32b), // doesn't matter what we put here because
                                                             // the entire cacheline is invalidated
                            IRB.getInt32(LSC_DATA_ELEMS_1), IRB.getInt32(CacheCtrl)});
      }
    }
}

void InlineRaytracing::StopAndStartRayquery(RTBuilder &IRB, Instruction *I, Value *rqObject, bool doSpillFill,
                                            bool doRQCheckRelease) {
  IRB.SetInsertPoint(I);
  Value *liveStack = {};

  if (doSpillFill) {
    // spill the raytracing stack to the register file
    auto *stackPtr = getStackPtr(IRB, rqObject);

    liveStack = IRB.CreateLoad(IRB.getRTStack2Ty(), stackPtr);

    // handle cache control
    InsertCacheControl(IRB, stackPtr);
  }

  // handle rayquery release
  if (doRQCheckRelease)
    IRB.CreateRayQueryReleaseIntrinsic();

  IRB.SetInsertPoint(I->getNextNode());

  // handle rayquery check
  if (doRQCheckRelease)
    IRB.CreateRayQueryCheckIntrinsic();

  if (doSpillFill) {
    // fill the raytracing stack from the register file
    auto *stackPtr = getStackPtr(IRB, rqObject);

    IRB.CreateStore(liveStack, stackPtr);
  }
}

void InlineRaytracing::HandleOptimizationsAndSpills(llvm::Function &F, LivenessDataMap &livenessDataMap) {
  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);

  SmallVector<Instruction *> continuationInstructions;
  SmallVector<Instruction *> indirectCallInstructions;
  SmallVector<Instruction *> hiddenCFInstructions;
  SmallVector<Instruction *> barrierInstructions;

  for (auto &I : instructions(F)) {
    if (isa<ContinuationHLIntrinsic>(&I))
      continuationInstructions.push_back(&I);
    else if (isUserFunctionCall(&I))
      indirectCallInstructions.push_back(&I);
    else if (isHidingComplexControlFlow(&I))
      hiddenCFInstructions.push_back(&I);
    else if (isBarrierIntrinsic(&I))
      barrierInstructions.push_back(&I);
  }

  // TODO: if the platform cant support divergent control flow,
  // we can still try doing throttling if we determine the liveness has a single
  // end point LD->lifetimeEndInstructions.size() == 1 it is unclear if this
  // will help though
  bool doRQCheckRelease =
      m_pCGCtx->platform.allowDivergentControlFlowRayQueryCheckRelease() &&
      m_pCGCtx->platform.enableRayQueryThrottling(m_pCGCtx->getModuleMetaData()->compOpt.EnableDynamicRQManagement) &&
      m_numSlotsUsed == 1;

  MapVector<const Instruction *, SmallVector<std::function<void(RTBuilder &)>>> instructionClosures;
  MapVector<LivenessData::Edge, SmallVector<std::function<void(RTBuilder &)>>> edgeClosures;

  for (auto &entry : livenessDataMap) {

    auto *rqObject = entry.first;
    auto *LD = &entry.second;

    // process the allocation acquire point
    // handle rayquery check
    instructionClosures[LD->lifetimeStart.inst].push_back([this, doRQCheckRelease](RTBuilder &IRB) {
      if (doRQCheckRelease)
        IRB.CreateRayQueryCheckIntrinsic();
    });

    // process the allocation release points
    for (auto &LE : LD->lifetimeEndInstructions) {
      auto *I = LE.inst;
      instructionClosures[isa<ReturnInst>(I) ? I : I->getNextNode()].push_back(
          [this, rqObject, doRQCheckRelease](RTBuilder &IRB) {
            auto *stackPtr = getStackPtr(IRB, IRB.Insert(rqObject->clone()));

            // handle cache control
            InsertCacheControl(IRB, stackPtr);

            // handle rayquery release
            if (doRQCheckRelease)
              IRB.CreateRayQueryReleaseIntrinsic();
          });
    }

    for (const auto &edge : LD->lifetimeEndEdges) {

      edgeClosures[edge].push_back([this, rqObject, doRQCheckRelease](RTBuilder &IRB) {
        auto *stackPtr = getStackPtr(IRB, IRB.Insert(rqObject->clone()));

        // handle cache control
        InsertCacheControl(IRB, stackPtr);

        // handle rayquery release
        if (doRQCheckRelease)
          IRB.CreateRayQueryReleaseIntrinsic();
      });
    }

    // handle continuation instructions
    for (auto *I : continuationInstructions) {

      if (!LD->ContainsInstruction(I))
        continue;

      instructionClosures[I].push_back([this, rqObject, doRQCheckRelease, I](RTBuilder &IRB) {
        if (m_pCGCtx->platform.hasEfficient64bEnabled())
          // efficient 64b comes with sync stack id support
          // so we don't have to spill the rtstack across BTD calls
          StopAndStartRayquery(IRB, I, IRB.Insert(rqObject->clone()), false, doRQCheckRelease);
        else
          StopAndStartRayquery(IRB, I, IRB.Insert(rqObject->clone()), true, doRQCheckRelease);
      });
    }

    // handle indirect calls
    for (auto *I : indirectCallInstructions) {

      if (!LD->ContainsInstruction(I))
        continue;

      instructionClosures[I].push_back([this, rqObject, doRQCheckRelease, I](RTBuilder &IRB) {
        StopAndStartRayquery(IRB, I, IRB.Insert(rqObject->clone()), true, doRQCheckRelease);
      });
    }

    // handle hidden control flow instructions
    for (auto *I : hiddenCFInstructions) {

      if (!LD->ContainsInstruction(I))
        continue;

      instructionClosures[I].push_back([this, rqObject, doRQCheckRelease, I](RTBuilder &IRB) {
        StopAndStartRayquery(IRB, I, IRB.Insert(rqObject->clone()), true, doRQCheckRelease);
      });
    }

    // handle barriers
    for (auto *I : barrierInstructions) {

      if (!LD->ContainsInstruction(I))
        continue;

      instructionClosures[I].push_back([this, rqObject, doRQCheckRelease, I](RTBuilder &IRB) {
        StopAndStartRayquery(IRB, I, IRB.Insert(rqObject->clone()), false, doRQCheckRelease);
      });
    }
  }

  for (const auto &[I, closures] : instructionClosures) {

    IRB.SetInsertPoint(const_cast<Instruction *>(I));
    for (const auto &c : closures)
      c(IRB);
  }

  for (const auto &[edge, closures] : edgeClosures) {

    auto *succ = edge.to;
    // to avoid multiple executions of rayquery release instructions,
    // we need to ensure that the "to" block has a single predecessor
    if (!edge.to->getSinglePredecessor())
      succ = SplitEdge(edge.from, succ);

    IRB.SetInsertPoint(succ->getFirstNonPHI());

    for (const auto &c : closures)
      c(IRB);
  }
}

void InlineRaytracing::LowerSlotAssignments(Function &F) {
  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);
  SmallVector<Instruction *> createRQInstructions;

  for (auto &U : m_Functions[CREATE_RQ_OBJECT]->uses())
    createRQInstructions.push_back(cast<Instruction>(U.getUser()));

  for (auto *I : createRQInstructions) {
    IRB.SetInsertPoint(I);
    auto *rqObject = IRB.CreateAlloca(m_RQObjectType);
    auto *slotPtr = getAtIndexFromRayQueryObject(IRB, rqObject, 0);
    IRB.CreateStore(I->getOperand(0), slotPtr);

    I->replaceAllUsesWith(rqObject);
  }

  llvm::for_each(createRQInstructions, [](Instruction *I) { I->eraseFromParent(); });
}

void InlineRaytracing::LowerStackPtrs(Function &F) {
  RTBuilder IRB(&*F.getEntryBlock().begin(), *m_pCGCtx);
  SmallVector<Instruction *> stackPtrInstructions;

  for (auto &U : m_Functions[GET_STACK_POINTER_FROM_RQ_OBJECT]->uses())
    stackPtrInstructions.push_back(cast<Instruction>(U.getUser()));

  for (auto *I : stackPtrInstructions) {
    IRB.SetInsertPoint(I);
    auto *stackPtr = IRB.getSyncStackPointer(getGlobalBufferPtr(IRB, I->getOperand(0)));
    I->replaceAllUsesWith(stackPtr);
  }

  llvm::for_each(stackPtrInstructions, [](Instruction *I) { I->eraseFromParent(); });
}

bool InlineRaytracing::runOnFunction(Function &F) {
  m_pCGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  if (!m_pCGCtx->hasSyncRTCalls())
    return false;

  for (auto &fn : m_Functions) {
    IGC_ASSERT_MESSAGE(!fn, "Function leaked?");
    if (fn) {
      fn->eraseFromParent();
      fn = nullptr;
    }
  }

  if (!LowerAllocations(F))
    return false;

  LowerIntrinsics(F);

  // TODO: we should split the pass here into two
  // 1. to reduce work for liveness analysis, we can run simplifyCFG here
  // 2. we wouldn't have to explicitly invalidate the analysis passes here
  DT.recalculate(F);

  getAnalysis<LoopInfoWrapperPass>().releaseMemory();
  getAnalysis<LoopInfoWrapperPass>().runOnFunction(F);
  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();

  auto livenessData = AnalyzeLiveness(F, DT, LI);
  AssignSlots(F, livenessData);
  HandleOptimizationsAndSpills(F, livenessData);
  LowerSlotAssignments(F);
  LowerStackPtrs(F);

  // set relevant metadata
  auto *MMD = m_pCGCtx->getModuleMetaData();

  MMD->FuncMD[&F].rtInfo.numSyncRTStacks = m_numSlotsUsed;
  MMD->rtInfo.numSyncRTStacks = std::max(MMD->rtInfo.numSyncRTStacks, m_numSlotsUsed);
  MMD->FuncMD[&F].hasSyncRTCalls = true;

  for (auto &fn : m_Functions) {
    IGC_ASSERT_MESSAGE(fn->use_empty(), "Function leaked?");
    if (fn) {
      fn->eraseFromParent();
      fn = nullptr;
    }
  }

  DumpLLVMIR(m_pCGCtx, "InlineRaytracing");

  IGC_ASSERT(verifyFunction(F, &dbgs()) == false);

  return true;
}

// Register pass to igc-opt
IGC_INITIALIZE_PASS_BEGIN(InlineRaytracing, "igc-inline-raytracing", "Handle inline raytracing", false, false)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(InlineRaytracing, "igc-inline-raytracing", "Handle inline raytracing", false, false)

char InlineRaytracing::ID = 0;

InlineRaytracing::InlineRaytracing() : AllocationLivenessAnalyzer(ID) {
  initializeInlineRaytracingPass(*PassRegistry::getPassRegistry());
}

namespace IGC {
Pass *createInlineRaytracing() { return new InlineRaytracing(); }
} // namespace IGC
