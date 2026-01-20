/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A subclass of IRBuilder that provides methods for raytracing functionality.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include <type_traits>
#include "RTBuilder.h"
#include "RTArgs.h"
#include "iStdLib/utility.h"
#include "common/debug/DebugMacros.hpp"
#include "common/MDFrameWork.h"
#include "common/igc_regkeys.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "RTStackFormat.h"
#include "Probe/Assertion.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvmWrapper/ADT/Optional.h>
#include "llvmWrapper/IR/Argument.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include <llvm/IR/Verifier.h>
#include "common/LLVMWarningsPop.hpp"

#include <optional>


using namespace llvm;
using namespace RTStackFormat;
using namespace IGC;

namespace {
class VAdapt {
  Value *VA = nullptr;

public:
  explicit VAdapt(std::nullptr_t) = delete;
  explicit VAdapt(Value *V) : VA(V) {}
  explicit VAdapt(IRBuilder<> &IRB, uint32_t V) : VA(IRB.getInt32(V)) {}
  explicit VAdapt(IRBuilder<> &IRB, uint64_t V) : VA(IRB.getInt64(V)) {}
  explicit VAdapt(IRBuilder<> &IRB, bool V) : VA(IRB.getInt1(V)) {}
  explicit VAdapt(IRBuilder<> &IRB, IGC::CallableShaderTypeMD V) : VA(IRB.getInt32(V)) {}
  operator Value *() const { return VA; }
};
}; // namespace

std::pair<BasicBlock *, BasicBlock *> RTBuilder::createTriangleFlow(Value *Cond, Instruction *InsertPoint,
                                                                    const Twine &TrueBBName, const Twine &JoinBBName) {
  auto &C = InsertPoint->getContext();
  auto *StartBB = InsertPoint->getParent();
  auto *JoinBB = StartBB->splitBasicBlock(InsertPoint, JoinBBName);

  auto *F = InsertPoint->getFunction();
  auto *TrueBB = BasicBlock::Create(C, TrueBBName, F, JoinBB);
  this->SetInsertPoint(TrueBB);
  this->CreateBr(JoinBB);

  StartBB->getTerminator()->eraseFromParent();
  this->SetInsertPoint(StartBB);
  this->CreateCondBr(Cond, TrueBB, JoinBB);

  return std::make_pair(TrueBB, JoinBB);
}

void RTBuilder::setInvariantLoad(LoadInst *LI) {
  auto *EmptyNode = MDNode::get(LI->getContext(), nullptr);
  if (IGC_IS_FLAG_DISABLED(DisableInvariantLoad))
    LI->setMetadata(LLVMContext::MD_invariant_load, EmptyNode);
}

Value *RTBuilder::getRtMemBasePtr(Value *globalBufferPtr) {
#define STYLE(X)                                                                                                       \
  {                                                                                                                    \
    using T = std::conditional_t<std::is_same_v<RTStackFormat::X, RTStackFormat::Xe>, RayDispatchGlobalData::RT::Xe,   \
                                 RayDispatchGlobalData::RT::Xe3>;                                                      \
    static_assert(offsetof(RayDispatchGlobalData::RT::Xe, rtMemBasePtr) == offsetof(T, rtMemBasePtr));                 \
  }
#include "RayTracingMemoryStyle.h"
#undef STYLE
  return globalBufferPtr ? _get_rtMemBasePtr_fromGlobals_Xe(globalBufferPtr, VALUE_NAME("rtMemBasePtr"))
                         : _get_rtMemBasePtr_Xe(VALUE_NAME("rtMemBasePtr"));
}

Value *RTBuilder::getStackSizePerRay(void) { return _get_stackSizePerRay_Xe(VALUE_NAME("stackSizePerRay")); }

Value *RTBuilder::getNumDSSRTStacks(void) { return _get_numDSSRTStacks_Xe(VALUE_NAME("numDSSRTStacks")); }


Value *RTBuilder::getMaxBVHLevels(void) {
  if (!DisableRTGlobalsKnownValues)
    return this->getInt32(MAX_BVH_LEVELS);

  switch (getMemoryStyle()) {
  case RTMemoryStyle::Xe:
    return _get_maxBVHLevels_Xe(VALUE_NAME("maxBVHLevels"));
  case RTMemoryStyle::Xe3:
  case RTMemoryStyle::Xe3PEff64:
    return _get_maxBVHLevels_Xe3(VALUE_NAME("maxBVHLevels"));
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getStatelessScratchPtr(void) { return _get_statelessScratchPtr(VALUE_NAME("statelessScratchPtr")); }


Value *RTBuilder::getBaseSurfaceStatePointer(Value *rayDispatchGlobalDataPtr) {
  // For non-RT shaders, which use RayQuery GlobalBufferPointer is delivered
  // in pushConstants. It must be read and passed to this function.
  return _getBaseSurfaceStatePointerFromPointerToGlobals(rayDispatchGlobalDataPtr,
                                                         VALUE_NAME("BaseSurfaceStatePointerFromPointerToGlobals"));
}

Value *RTBuilder::getIsFrontFace(RTBuilder::StackPointerVal *StackPointer, Value *ShaderTy) {
  auto *isCommitted = CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit));
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getIsFrontFace_##X(StackPointer, isCommitted, VALUE_NAME("is_front_face"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}


Value *RTBuilder::CreateSyncStackPtrIntrinsic(Value *Addr, Type *PtrTy, bool AddDecoration) {
  Module *M = this->GetInsertBlock()->getModule();
  Type *Tys[] = {PtrTy, Addr->getType()};
  CallInst *StackPtr = this->CreateCall(GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_SyncStackPtr, Tys),
                                        Addr, VALUE_NAME("perLaneSyncStackPointer"));

  if (AddDecoration) {
    this->setReturnAlignment(StackPtr, RTStackAlign);
    if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes)) {
      this->setDereferenceable(StackPtr, getRTStack2Size());
    }
  }

  return StackPtr;
}

RayQueryCheckIntrinsic *RTBuilder::CreateRayQueryCheckIntrinsic(Value *predicate) {
  Module *M = this->GetInsertBlock()->getModule();

  if (!predicate)
    predicate = getTrue();

  Value *rayQueryCheck =
      CreateCall(GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_RayQueryCheck), predicate);

  return cast<RayQueryCheckIntrinsic>(rayQueryCheck);
}

RayQueryReleaseIntrinsic *RTBuilder::CreateRayQueryReleaseIntrinsic(Value *predicate) {
  Module *M = this->GetInsertBlock()->getModule();

  if (!predicate)
    predicate = getTrue();

  Value *rayQueryRelease =
      CreateCall(GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_RayQueryRelease), predicate);
  return cast<RayQueryReleaseIntrinsic>(rayQueryRelease);
}

PreemptionDisableIntrinsic *RTBuilder::CreatePreemptionDisableIntrinsic() {
  Module *M = this->GetInsertBlock()->getModule();

  auto *GII = CreateCall(GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_PreemptionDisable));

  return cast<PreemptionDisableIntrinsic>(GII);
}

PreemptionEnableIntrinsic *RTBuilder::CreatePreemptionEnableIntrinsic(Value *Flag) {
  Module *M = this->GetInsertBlock()->getModule();

  auto *GII =
      CreateCall(GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_PreemptionEnable), Flag ? Flag : getTrue());

  return cast<PreemptionEnableIntrinsic>(GII);
}


Value *RTBuilder::getGlobalSyncStackID() {
  // global sync stack id = dssIDGlobal * NUM_SIMD_LANES_PER_DSS + SyncStackID
  Value *IDGlobal = this->getGlobalDSSID();
  Value *numSimdLanesPerDss = this->getInt32(Ctx.platform.getRTStackDSSMultiplier());

  Value *val = this->CreateMul(IDGlobal, numSimdLanesPerDss);

  Value *stackID = this->getSyncStackID();
  val = this->CreateAdd(val, this->CreateZExt(stackID, this->getInt32Ty()), VALUE_NAME("globalSyncStackID"));

  return val;
}

// This is for new RT stack layout.
// The base itself is pointing to the beginning of the memory block containing RTStack for 4 consecutive stackIDs.
// Here we calculating the base sync stackID for such 4-tuple,
// that is, the expression in parentheses from this formula:
// base_sync = rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + 4 * (stackID / 4) + 4) * syncStackSize
Value *RTBuilder::getGlobalSyncStackIDBase() {
  Value *IDGlobal = this->getGlobalDSSID();
  Value *numSimdLanesPerDss = this->getInt32(Ctx.platform.getRTStackDSSMultiplier());

  Value *val = this->CreateMul(IDGlobal, numSimdLanesPerDss);

  Value *stackID = this->getSyncStackID();
  // 4*(stackID/4) == stackID & ~(3)
  Value *stackIDBase =
      this->CreateAnd(CreateZExt(stackID, val->getType()),
                      ConstantInt::get(val->getType(), ~uint64_t(3),
                                       true /* sign extend to propagate MSB `1` to a wider type in necessary */));
  stackIDBase = this->CreateAdd(stackIDBase, ConstantInt::get(val->getType(), 4));

  val = this->CreateAdd(val, stackIDBase, VALUE_NAME("globalSyncStackIDBase"));

  return val;
}

uint32_t RTBuilder::getRTStack2Size() const {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return sizeofRTStack2<RTStack2<RTStackFormat::X>>();
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return 0;
}

Value *RTBuilder::getRTStackSize(uint32_t Align) {
  // syncStackSize = sizeof(HitInfo)*2 + (sizeof(Ray) + sizeof(TravStack))*RTDispatchGlobals.maxBVHLevels
  Value *stackSize =
      this->CreateMul(this->getInt32(sizeof(MemRay<Xe>) + sizeof(MemTravStack)), this->getMaxBVHLevels());

  stackSize = this->CreateAdd(stackSize, this->getInt32(sizeof(MemHit<Xe>) * 2), VALUE_NAME("RTStackSize"));

  stackSize = this->alignVal(stackSize, Align);

  return stackSize;
}

uint32_t RTBuilder::getRTStackSectorSize(uint32_t Align) {
  // Stack Layout Optimization introduced a new memory layout,
  // where Stack for given StackID is divided into two sectors:
  //
  // +----------------+
  // | Committed Hit  |
  // | Short Stack 0  |
  // | Ray 0          |
  // +----------------+
  // | Potential Hit  |
  // | Short Stack 1  |
  // | Ray 1          |
  // +----------------+
  //
  // This function returns the size of each sector.
  uint32_t stackSectorSize = sizeof(MemRay<Xe>) + sizeof(MemTravStack) + sizeof(MemHit<Xe>);

  stackSectorSize =
      static_cast<uint32_t>(cast<ConstantInt>(this->alignVal(this->getInt32(stackSectorSize), Align))->getZExtValue());

  return stackSectorSize;
}

Value *RTBuilder::getSyncRTStackSize() { return this->getRTStackSize(RayDispatchGlobalData::StackChunkSize); }

Value *RTBuilder::getSyncStackOffset(bool rtMemBasePtr) {
  // Per thread Synchronous RTStack address/ptr is calculated using the following formula
  // (note that offset is calculated here so RTDispatchGlobals.rtMemBasePtr is not taken into account):
  // syncBase = RTDispatchGlobals.rtMemBasePtr - (GlobalSyncStackID + 1) * syncStackSize;
  // If we start from syncStack, the address/ptr should be:
  // syncBase = syncStack + (NumSyncStackSlots - (GlobalSyncStackID + 1)) * syncStackSize
  // For new stack layout, enabled on XE3P with Efficient64b mode enabled, they are:
  // base_sync = RTDispatchGlobals.rtMemBasePtr - (DSSID * NUM_SIMD_LANES_PER_DSS + 4 * (stackID / 4) + 4) *
  // syncStackSize base128_sync = base_sync + (stackID % 4) * 128 And we return offset part of base128_sync address. If
  // we start from syncStack, the address/ptr should be: base128_sync = syncStack + (NumSyncStackSlots - (DSSID *
  // NUM_SIMD_LANES_PER_DSS + 4 * (stackID / 4) + 4)) *
  //     syncStackSize - ((stackID % 4) * 128) = syncStack + NumSyncStackSlots * syncStackSize - base128Sync

  if (Ctx.platform.hasEfficient64bEnabled()) {
    // In new StackLayout memory for StackIDs is organized in chunks
    // of 4 StackIDs.The offset for particular StackID should be calculated
    // as follows:
    // 1. Calculate the 4 - StackIDs block index(in StackIDs units).
    // 2. In case of Stateful access the 4 - StackIDs block index is
    // subtracted from the maximum number of stacks.It's necessary as
    // in Stateful access, the address of the sync stack is taken from
    // SurfaceState, where it points to the beginning of the stack, while
    // SyncStack supposed to start from the end of the Sync Stack.
    // So we need to calculate the offset, which will be added to the address
    // of the Stack from SurfaceState.
    // 2. Multiply it by the size of memory for one StackID to get the
    // byte offset for the beginning of 4 - StackIDs block.
    // 3. For the Stateful access, add the byte offset for the particular StackID
    // from the 4 - StackID block.This way we get the exact offset to be added
    // to the Stack address from the SurfaceState.
    // For the Stateless access, we need to calculate the offset value, which will
    // be subtracted from the StackAddress, as in this mode the SyncStack pointer
    // points to the end of the SyncStack.Thus the offset to the particular StackID
    // in 4 - StackID block needs to be subtracted from the 4 - StackID block offset, so
    // when the calculated offset is subtracted from the syncStack pointer, the higher
    // StackIDs get higher address.
    //
    // +-----------+
    // | Stack memory start | <- Stateful pointer
    // ...
    // +-----------+
    // | StackID 4 |
    // | StackID 5 |
    // | StackID 6 |
    // | StackID 7 |
    // +-----------+
    // | StackID 0 |
    // | StackID 1 |
    // | StackID 2 |
    // | StackID 3 |
    // +-----------+
    // | Stack memory end | <- Stateless pointer

    Value *globalStackIDBase = this->getGlobalSyncStackIDBase();

    if (!rtMemBasePtr) {
      globalStackIDBase =
          this->CreateSub(ConstantInt::get(globalStackIDBase->getType(), getNumSyncStackSlots()), globalStackIDBase);
    }

    Value *stackSize = this->getSyncRTStackSize();
    Value *baseForBlockInBytes = this->CreateMul(globalStackIDBase, stackSize);

    // (stackID % 4) = stackID & 0x3
    Value *stackIDTail = this->getSyncStackID();
    stackIDTail = this->CreateAnd(stackIDTail, ConstantInt::get(stackIDTail->getType(), 3));

    Value *bytesToAdd =
        this->CreateMul(stackIDTail, getInt32(this->getRTStackSectorSize(RayDispatchGlobalData::StackChunkSize)));

    Value *base128Sync = nullptr;

    if (!rtMemBasePtr) {
      base128Sync = this->CreateAdd(baseForBlockInBytes, bytesToAdd);
    } else {
      base128Sync = this->CreateSub(baseForBlockInBytes, bytesToAdd);
    }

    return base128Sync;
  }

  Value *globalStackID = this->getGlobalSyncStackID();
  Value *OffsetID = this->CreateAdd(globalStackID, this->getInt32(1));

  if (!rtMemBasePtr) {
    OffsetID = this->CreateSub(this->getInt32(getNumSyncStackSlots()), OffsetID);
  }

  Value *stackSize = this->getSyncRTStackSize();

  return this->CreateMul(this->CreateZExt(OffsetID, this->getInt32Ty()),
                         this->CreateZExt(stackSize, this->getInt32Ty()), VALUE_NAME("SyncStackOffset"));
}

RTBuilder::SyncStackPointerVal *RTBuilder::getSyncStackPointer(RTBuilder::RTMemoryAccessMode Mode,
                                                               Value *syncStackOffset, Value *globalBufferPtr) {
  auto *PointeeTy = getRTStack2Ty();
  if (Mode == RTBuilder::STATEFUL) {
    IGC_ASSERT_MESSAGE(!globalBufferPtr, "Arbitrary global buffer is not supported in stateful mode");
    uint32_t AddrSpace = getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData());
    Value *perLaneStackPointer = this->CreateIntToPtr(syncStackOffset, PointerType::get(PointeeTy, AddrSpace));
    return static_cast<RTBuilder::SyncStackPointerVal *>(perLaneStackPointer);
  } else {
    Value *memBasePtr = nullptr;
    {
      memBasePtr = this->getRtMemBasePtr(globalBufferPtr);
    }
    IGC_ASSERT(memBasePtr != nullptr);

    Value *stackBase = this->CreatePtrToInt(memBasePtr, this->getInt64Ty());
    Value *perLaneStackPointer = this->CreateSub(stackBase, this->CreateZExt(syncStackOffset, stackBase->getType()),
                                                 VALUE_NAME("HWMem.perLaneSyncStackPointer"));
    perLaneStackPointer = this->CreateIntToPtr(perLaneStackPointer, PointerType::get(PointeeTy, ADDRESS_SPACE_GLOBAL));
    return static_cast<RTBuilder::SyncStackPointerVal *>(perLaneStackPointer);
  }
}

// forceRTStackAccessMode is an optional parameter which allows to
// override the global settings.
RTBuilder::SyncStackPointerVal *
RTBuilder::getSyncStackPointer(Value *globalBufferPtr,
                               std::optional<RTBuilder::RTMemoryAccessMode> forceRTStackAccessMode) {
  auto Mode =
      Ctx.getModuleMetaData()->rtInfo.RTSyncStackSurfaceStateOffset ? RTBuilder::STATEFUL : RTBuilder::STATELESS;

  if (forceRTStackAccessMode.has_value()) {
    Mode = *forceRTStackAccessMode;
  }

  // requests for the sync stack pointer in early phases of compilation
  // will return a marker intrinsic that can be analyzed later by cache ctrl pass.
  // "marker" here means this intrinsic will be lowered to almost nothing eventually.
  auto *PtrTy = this->getRTStack2PtrTy(Mode, false);

  Value *stackOffset = this->getSyncStackOffset(RTBuilder::STATELESS == Mode);
  stackOffset = this->getSyncStackPointer(Mode, stackOffset, globalBufferPtr);
  return static_cast<RTBuilder::SyncStackPointerVal *>(this->CreateSyncStackPtrIntrinsic(stackOffset, PtrTy, true));
}


TraceRayIntrinsic *RTBuilder::createTraceRay(Value *bvhLevel, Value *traceRayCtrl, bool isRayQuery,
                                             Value *globalBufferPointer, const Twine &PayloadName) {
  Module *module = this->GetInsertBlock()->getModule();

  if (!globalBufferPointer)
    globalBufferPointer = this->getGlobalBufferPtr();

  GenISAIntrinsic::ID ID = isRayQuery ? GenISAIntrinsic::GenISA_TraceRaySync : GenISAIntrinsic::GenISA_TraceRayAsync;

  Function *traceFn = GenISAIntrinsic::getDeclaration(module, ID, globalBufferPointer->getType());

  Value *payload = getTraceRayPayload(bvhLevel, traceRayCtrl, isRayQuery, PayloadName);

  SmallVector<Value *, 4> Args{globalBufferPointer, payload};


  return cast<TraceRayIntrinsic>(this->CreateCall(traceFn, Args));
}

void RTBuilder::createReadSyncTraceRay(Value *val) {
  Function *readFunc =
      GenISAIntrinsic::getDeclaration(this->GetInsertBlock()->getModule(), GenISAIntrinsic::GenISA_ReadTraceRaySync);

  this->CreateCall(readFunc, val);
}

TraceRaySyncIntrinsic *RTBuilder::createSyncTraceRay(Value *bvhLevel, Value *traceRayCtrl, Value *globalBufferPointer,
                                                     const Twine &PayloadName) {
  return cast<TraceRaySyncIntrinsic>(createTraceRay(bvhLevel, this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true,
                                                    globalBufferPointer, PayloadName));
}

TraceRaySyncIntrinsic *RTBuilder::createSyncTraceRay(uint32_t bvhLevel, Value *traceRayCtrl, Value *globalBufferPointer,
                                                     const Twine &PayloadName) {
  return cast<TraceRaySyncIntrinsic>(createTraceRay(this->getInt32(bvhLevel),
                                                    this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true,
                                                    globalBufferPointer, PayloadName));
}


static BasicBlock *getUnsetPhiBlock(PHINode *PN) {
  SmallPtrSet<BasicBlock *, 4> BBs;

  for (auto *BB : PN->blocks())
    BBs.insert(BB);

  for (auto *BB : predecessors(PN->getParent())) {
    if (BBs.count(BB) == 0)
      return BB;
  }

  return nullptr;
}

// If MemHit is invalid (below if case), workload might still try to access data with InstanceLeafPtr which is not set
// and is a dangling pointer. Even though this workload behavior is against API spec, but we should not "crash".
// To avoid this crash, we check if the data request is invalid here, if it's invalid, we return an invalid data
// (uint32: 0xFFFFFFFF, float: INF)
//  if (forCommitted && committedHit::valid == 0) ||
//      (!forCommitted && potentialHit::leafType == NODE_TYPE_INVALID))
//     value = UINT_MAX/0
//  else
//     get value as usual
//----------------------------------
//% valid36 = ...
// br i1 % valid36, label% validLeafBB, label% invalidLeafBB
//
// validLeafBB : ; preds = % ProceedEndBlock
//; later, insert logic to get valid value with InstanceLeafPtr
// br label% validEndBlock
//
// invalidLeafBB : ; preds = % ProceedEndBlock
// br label % validEndBlock
//
// validEndBlock : ; preds = % invalidLeafBB, % validLeafBB
//% 97 = phi i32[-1, % invalidLeafBB] //later, valid value should be inserted here
//----------------------------------
// return value:
//   BasicBlock* is validLeafBB to let caller insert logic to this BB
//   PHINode*    is %97 above to let caller add another incoming node for phi
std::pair<BasicBlock *, PHINode *> RTBuilder::validateInstanceLeafPtr(RTBuilder::StackPointerVal *perLaneStackPtr,
                                                                      Instruction *I, Value *forCommitted) {
  Instruction *trueTerm = nullptr;
  Instruction *falseTerm = nullptr;
  SplitBlockAndInsertIfThenElse(forCommitted, I, &trueTerm, &falseTerm);

  SetInsertPoint(trueTerm);
  auto *trueV = getHitValid(perLaneStackPtr, forCommitted);

  SetInsertPoint(falseTerm);
  auto *falseV = this->CreateICmpNE(getLeafType(perLaneStackPtr, forCommitted), this->getInt32(NODE_TYPE_INVALID),
                                    VALUE_NAME("validLeafType"));

  SetInsertPoint(I);
  auto *valid = CreatePHI(trueV->getType(), 2, VALUE_NAME("leafType"));
  valid->addIncoming(trueV, trueTerm->getParent());
  valid->addIncoming(falseV, falseTerm->getParent());

  auto &C = I->getContext();
  auto *CSBlock = I->getParent();
  auto *endBlock = CSBlock->splitBasicBlock(I, VALUE_NAME("validEndBlock"));
  Function *F = this->GetInsertBlock()->getParent();

  BasicBlock *validLeafBB = BasicBlock::Create(C, VALUE_NAME("validLeafBB"), F, endBlock);
  BasicBlock *invalidLeafBB = BasicBlock::Create(C, VALUE_NAME("invalidLeafBB"), F, endBlock);
  CSBlock->getTerminator()->eraseFromParent();
  this->SetInsertPoint(CSBlock);
  this->CreateCondBr(valid, validLeafBB, invalidLeafBB);

  this->SetInsertPoint(validLeafBB);
  this->CreateBr(endBlock);

  this->SetInsertPoint(invalidLeafBB);
  Value *invalidVal = nullptr;
  if (I->getType()->isIntegerTy(32))
    invalidVal = this->getInt32(IGC_GET_FLAG_VALUE(RTInValidDefaultIndex));
  else if (I->getType()->isFloatTy())
    invalidVal = ConstantFP::getInfinity(this->getFloatTy());
  else {
    IGC_ASSERT_MESSAGE(0, "Unsupported datatype.");
  }

  this->CreateBr(endBlock);

  // END block
  this->SetInsertPoint(I);
  PHINode *phi = this->CreatePHI(I->getType(), 2);
  phi->addIncoming(invalidVal, invalidLeafBB);

  return std::make_pair(validLeafBB, phi);
}

// Return an allocation to `size` number of RayQueryObject.
//  Note that we separate RTStack/SMStack and RTCtrl to help reduce the ShadowMemory size
//  by hoping mem2reg kicks in to lower RTCtrl to GRF
// For example:
//
// builder.createAllocaRayQueryObjects(5)
// ==>
//  if(ShrinkShadowMemoryIfNoSpillfill)
//      %"&ShadowMemory.RayQueryObjects".first = alloca [5 x %"struct.RTStackFormat::SMStack2"]
//  else
//      % "&ShadowMemory.RayQueryObjects".first = alloca[5 x % "struct.RTStackFormat::RTStack2"]
//  % "&ShadowMemory.RayQueryObjects".second = alloca[5 x int8]
//
std::pair<AllocaInst *, AllocaInst *> RTBuilder::createAllocaRayQueryObjects(unsigned int size, bool bShrinkSMStack,
                                                                             const llvm::Twine &Name) {
  // FIXME: Temp solution: to shrink ShadowMemory, if ShrinkShadowMemoryIfNoSpillfill is true (we know no spillfill),
  // then, we alloca SMStack instead of RTStack to reduce the size. Also, here, we simply cast SMStack pointer to
  // RTStack which is risky and it's safe now if we only shrink the last variable of RTStack (MemTravStack). But don't
  // shrink data in the middle of RTStack which will lead to holes.
  auto *RTStackTy = bShrinkSMStack ? getSMStack2Ty() : getRTStack2Ty();

  AllocaInst *rtStacks = this->CreateAlloca(ArrayType::get(RTStackTy, size), nullptr, Name);

  // FIXME: temp solution
  // one example is below, right now, we don't respect below DW alignment.
  // as a result, if we alloca <1xi8> here which alloca memory below %0, later, %0's store will have alignment issue
  // to WA this, let's temporily use DW, but sure, this won't solve all other issues.
  //   %0 = alloca <3 x float>
  //   store <3 x float> zeroinitializer, <3 x float>* %0
  AllocaInst *rtCtrls = this->CreateAlloca(ArrayType::get(this->getInt32Ty(), size), nullptr, Name);

  return std::make_pair(rtStacks, rtCtrls);
}

void RTBuilder::setDoneBit(RTBuilder::StackPointerVal *StackPointer, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _setDoneBit_##X(StackPointer, VAdapt{*this, Committed});                                                           \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}


uint32_t RTBuilder::getSyncStackSize() { return Ctx.getModuleMetaData()->rtInfo.RayQueryAllocSizeInBytes; }

uint32_t RTBuilder::getNumSyncStackSlots() {
  return MaxDualSubSlicesSupported * Ctx.platform.getRTStackDSSMultiplier();
}


Value *RTBuilder::alignVal(Value *V, uint64_t Align) {
  // Aligned = (V + Mask) & ~Mask;
  IGC_ASSERT_MESSAGE(iSTD::IsPowerOfTwo(Align), "Must be power-of-two!");
  IGC_ASSERT_MESSAGE(V->getType()->isIntegerTy(), "not an int?");

  uint32_t NumBits = V->getType()->getIntegerBitWidth();
  uint64_t Mask = Align - 1;
  Value *Aligned = this->CreateAnd(this->CreateAdd(V, this->getIntN(NumBits, Mask)), this->getIntN(NumBits, ~Mask),
                                   VALUE_NAME(V->getName() + Twine("-align-") + Twine(Align)));

  return Aligned;
}

Value *RTBuilder::getRayInfo(StackPointerVal *perLaneStackPtr, uint32_t Idx, uint32_t BvhLevel) {
  IGC_ASSERT_MESSAGE(Idx < 8, "out-of-bounds!");
  IGC_ASSERT(BvhLevel < 2);
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getRayInfo_##X(perLaneStackPtr, VAdapt{*this, Idx}, VAdapt{*this, BvhLevel},                               \
                           VALUE_NAME("RayInfo." + Twine(Idx)));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getRayTMin(RTBuilder::StackPointerVal *perLaneStackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getRayTMin_##X(perLaneStackPtr, VALUE_NAME("rayTMin"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

// returns an i16 value containing the current rayflags. Sync only.
Value *RTBuilder::getRayFlags(RTBuilder::SyncStackPointerVal *perLaneStackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getRayFlagsSync_##X(perLaneStackPtr, VALUE_NAME("rayflags"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

void RTBuilder::setRayFlags(RTBuilder::SyncStackPointerVal *perLaneStackPtr, Value *V) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _setRayFlagsSync_##X(perLaneStackPtr, V);                                                                          \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}


Value *RTBuilder::getWorldRayOrig(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getWorldRayOrig_##X(perLaneStackPtr, VAdapt{*this, dim}, VALUE_NAME("WorldRayOrig[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getMemRayOrig(StackPointerVal *perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine &Name) {
  IGC_ASSERT(BvhLevel < 2);
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getMemRayOrig_##X(perLaneStackPtr, VAdapt{*this, dim}, VAdapt{*this, BvhLevel}, Name);
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getMemRayDir(StackPointerVal *perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine &Name) {
  IGC_ASSERT(BvhLevel < 2);
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getMemRayDir_##X(perLaneStackPtr, VAdapt{*this, dim}, VAdapt{*this, BvhLevel}, Name);
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getWorldRayDir(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getWorldRayDir_##X(perLaneStackPtr, VAdapt{*this, dim}, VALUE_NAME("WorldRayDir[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getObjRayOrig(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy,
                                Instruction *I, bool checkInstanceLeafPtr) {

  auto *IP = &*GetInsertPoint();

  auto *oldBB = IP->getParent();
  auto *bb = SplitBlock(oldBB, IP);

  auto *isClosestHitBB = BasicBlock::Create(*Ctx.getLLVMContext(), VALUE_NAME("isClosestHitBB"), IP->getFunction(), bb);
  SetInsertPoint(isClosestHitBB);
  auto *isClosestHitBBTerm =
      CreateBr(bb); // we have to do this because the functions we call are going to split the block again...
  SetInsertPoint(isClosestHitBBTerm);
  auto *newI = I->clone();
  Insert(newI);
  auto *isClosestHitV = this->TransformWorldToObject(perLaneStackPtr, dim, true, ShaderTy, newI, checkInstanceLeafPtr);
  isClosestHitV->setName(VALUE_NAME("isClosestHitV"));
  newI->eraseFromParent();

  auto *isMissBB = BasicBlock::Create(Context, VALUE_NAME("isMissBB"), IP->getFunction(), bb);
  SetInsertPoint(isMissBB);
  auto *isMissBBTerm = CreateBr(bb);
  SetInsertPoint(isMissBBTerm);
  auto *isMissV = getWorldRayOrig(perLaneStackPtr, dim);
  isMissV->setName(VALUE_NAME("isMissV"));

  auto *defaultBB = BasicBlock::Create(Context, VALUE_NAME("default"), IP->getFunction(), bb);
  SetInsertPoint(defaultBB);
  auto *defaultBBTerm = CreateBr(bb);
  SetInsertPoint(defaultBBTerm);
  auto *defaultV = getMemRayOrig(perLaneStackPtr, dim, BOTTOM_LEVEL_BVH, VALUE_NAME("ObjRayOrig[" + Twine(dim) + "]"));
  defaultV->setName(VALUE_NAME("defaultV"));

  // create switch statement
  oldBB->getTerminator()->eraseFromParent();
  SetInsertPoint(oldBB);
  auto *switchI = CreateSwitch(ShaderTy, defaultBB);
  switchI->addCase(getInt32(Miss), isMissBB);
  switchI->addCase(getInt32(ClosestHit), isClosestHitBB);

  SetInsertPoint(IP);
  auto *info = CreatePHI(isClosestHitV->getType(), 3);
  info->addIncoming(isClosestHitV, isClosestHitBBTerm->getParent());
  info->addIncoming(isMissV, isMissBBTerm->getParent());
  info->addIncoming(defaultV, defaultBBTerm->getParent());

  return info;
}

Value *RTBuilder::getObjRayDir(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy,
                               Instruction *I, bool checkInstanceLeafPtr) {

  auto *IP = &*GetInsertPoint();

  auto *oldBB = IP->getParent();
  auto *bb = SplitBlock(oldBB, IP);

  auto *isClosestHitBB = BasicBlock::Create(*Ctx.getLLVMContext(), VALUE_NAME("isClosestHitBB"), IP->getFunction(), bb);
  SetInsertPoint(isClosestHitBB);
  auto *isClosestHitBBTerm =
      CreateBr(bb); // we have to do this because the functions we call are going to split the block again...
  SetInsertPoint(isClosestHitBBTerm);
  auto *newI = I->clone();
  Insert(newI);
  auto *isClosestHitV = this->TransformWorldToObject(perLaneStackPtr, dim, false, ShaderTy, newI, checkInstanceLeafPtr);
  newI->eraseFromParent();

  auto *isMissBB = BasicBlock::Create(Context, VALUE_NAME("isMissBB"), IP->getFunction(), bb);
  SetInsertPoint(isMissBB);
  auto *isMissBBTerm = CreateBr(bb);
  SetInsertPoint(isMissBBTerm);
  auto *isMissV = getWorldRayDir(perLaneStackPtr, dim);

  auto *defaultBB = BasicBlock::Create(Context, VALUE_NAME("default"), IP->getFunction(), bb);
  SetInsertPoint(defaultBB);
  auto *defaultBBTerm = CreateBr(bb);
  SetInsertPoint(defaultBBTerm);
  auto *defaultV = getMemRayDir(perLaneStackPtr, dim, BOTTOM_LEVEL_BVH, VALUE_NAME("ObjRayDir[" + Twine(dim) + "]"));

  // create switch statement
  oldBB->getTerminator()->eraseFromParent();
  SetInsertPoint(oldBB);
  auto *switchI = CreateSwitch(ShaderTy, defaultBB);
  switchI->addCase(getInt32(Miss), isMissBB);
  switchI->addCase(getInt32(ClosestHit), isClosestHitBB);

  SetInsertPoint(IP);
  auto *info = CreatePHI(isClosestHitV->getType(), 3);
  info->addIncoming(isClosestHitV, isClosestHitBBTerm->getParent());
  info->addIncoming(isMissV, isMissBBTerm->getParent());
  info->addIncoming(defaultV, defaultBBTerm->getParent());

  return info;
}

Value *RTBuilder::getRayTCurrent(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getRayTCurrent_##X(perLaneStackPtr, ShaderTy, VALUE_NAME("rayTCurrent"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getInstanceIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy, Instruction *I,
                                   bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    auto [ValidBB, PN] =
        validateInstanceLeafPtr(perLaneStackPtr, I, CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit)));
    this->SetInsertPoint(ValidBB->getTerminator());
    Value *validVal = getInstanceIndex(perLaneStackPtr, ShaderTy);
    PN->addIncoming(validVal, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return getInstanceIndex(perLaneStackPtr, ShaderTy);
  }
}

Value *RTBuilder::getInstanceIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getInstanceIndex_##X(perLaneStackPtr, ShaderTy, VALUE_NAME("InstanceIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getInstanceID(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy, Instruction *I,
                                bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    auto [ValidBB, PN] =
        validateInstanceLeafPtr(perLaneStackPtr, I, CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit)));
    this->SetInsertPoint(ValidBB->getTerminator());
    Value *validVal = getInstanceID(perLaneStackPtr, ShaderTy);
    PN->addIncoming(validVal, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return getInstanceID(perLaneStackPtr, ShaderTy);
  }
}

Value *RTBuilder::getInstanceID(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getInstanceID_##X(perLaneStackPtr, ShaderTy, VALUE_NAME("InstanceID"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getPrimitiveIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Instruction *I, Value *leafType,
                                    Value *ShaderTy, bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    auto [ValidBB, PN] =
        validateInstanceLeafPtr(perLaneStackPtr, I, CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit)));
    this->SetInsertPoint(ValidBB->getTerminator());
    auto *validPrimIndex = getPrimitiveIndex(perLaneStackPtr, leafType, ShaderTy);
    PN->addIncoming(validPrimIndex, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return getPrimitiveIndex(perLaneStackPtr, leafType, ShaderTy);
  }
}

Value *RTBuilder::getPrimitiveIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *leafType, Value *ShaderTy) {
    switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getPrimitiveIndex_##X(perLaneStackPtr, leafType, ShaderTy, VALUE_NAME("primitiveIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return nullptr;
}


Value *RTBuilder::getGeometryIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Instruction *I, Value *leafType,
                                   Value *ShaderTy, bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    auto [ValidBB, PN] =
        validateInstanceLeafPtr(perLaneStackPtr, I, CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit)));
    this->SetInsertPoint(ValidBB->getTerminator());
    Value *validGeomIndex = getGeometryIndex(perLaneStackPtr, leafType, ShaderTy);
    PN->addIncoming(validGeomIndex, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return getGeometryIndex(perLaneStackPtr, leafType, ShaderTy);
  }
}

Value *RTBuilder::getGeometryIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *leafType, Value *ShaderTy) {
    switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getGeometryIndex_##X(perLaneStackPtr, leafType, ShaderTy, VALUE_NAME("geometryIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return nullptr;
}

Value *RTBuilder::getInstanceContributionToHitGroupIndex(RTBuilder::StackPointerVal *perLaneStackPtr, Value *ShaderTy) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getInstanceContributionToHitGroupIndex_##X(perLaneStackPtr, ShaderTy,                                      \
                                                       VALUE_NAME("InstanceContributionToHitGroupIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getRayMask(RTBuilder::StackPointerVal *perLaneStackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getRayMask_##X(perLaneStackPtr, VALUE_NAME("RayMask"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getObjToWorld(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy,
                                Instruction *I, bool checkInstanceLeafPtr) {
  return getObjWorldAndWorldObj(perLaneStackPtr, IGC::OBJECT_TO_WORLD, dim, ShaderTy, I, checkInstanceLeafPtr);
}

Value *RTBuilder::getWorldToObj(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t dim, Value *ShaderTy,
                                Instruction *I, bool checkInstanceLeafPtr) {
  return getObjWorldAndWorldObj(perLaneStackPtr, IGC::WORLD_TO_OBJECT, dim, ShaderTy, I, checkInstanceLeafPtr);
}

Value *RTBuilder::getObjWorldAndWorldObj(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t infoKind, uint32_t dim,
                                         Value *ShaderTy, Instruction *I, bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    auto [ValidBB, PN] =
        validateInstanceLeafPtr(perLaneStackPtr, I, CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit)));
    this->SetInsertPoint(ValidBB->getTerminator());
    Value *validVal = getObjWorldAndWorldObj(perLaneStackPtr, infoKind, dim, ShaderTy);
    PN->addIncoming(validVal, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return getObjWorldAndWorldObj(perLaneStackPtr, infoKind, dim, ShaderTy);
  }
}

Value *RTBuilder::getObjWorldAndWorldObj(RTBuilder::StackPointerVal *perLaneStackPtr, uint32_t infoKind, uint32_t dim,
                                         Value *ShaderTy) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getObjWorldAndWorldObj_##X(perLaneStackPtr, VAdapt{*this, dim},                                            \
                                       VAdapt{*this, infoKind == IGC::OBJECT_TO_WORLD}, ShaderTy,                      \
                                       VALUE_NAME("matrix[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::TransformWorldToObject(RTBuilder::StackPointerVal *perLaneStackPtr, unsigned int dim, bool isOrigin,
                                         Value *ShaderTy, Instruction *I, bool checkInstanceLeafPtr) {
  if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr)) {
    this->SetInsertPoint(I);
    auto *forCommitted = CreateICmpEQ(ShaderTy, getInt32(CallableShaderTypeMD::ClosestHit),
                                      VALUE_NAME("TransformWorldToObjectShaderTy"));
    auto [ValidBB, PN] = validateInstanceLeafPtr(perLaneStackPtr, I, forCommitted);
    this->SetInsertPoint(ValidBB->getTerminator());
    Value *validVal = TransformWorldToObject(perLaneStackPtr, dim, isOrigin, ShaderTy);
    PN->addIncoming(validVal, getUnsetPhiBlock(PN));
    this->SetInsertPoint(I);
    return PN;
  } else {
    return TransformWorldToObject(perLaneStackPtr, dim, isOrigin, ShaderTy);
  }
}

Value *RTBuilder::TransformWorldToObject(RTBuilder::StackPointerVal *perLaneStackPtr, unsigned int dim, bool isOrigin,
                                         Value *ShaderTy) {
  IGC_ASSERT_MESSAGE((dim < 3), "dim out of bounds!");

  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _TransformWorldToObject_##X(perLaneStackPtr, VAdapt{*this, dim}, VAdapt{*this, isOrigin}, ShaderTy);
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}


Value *RTBuilder::getTraceRayPayload(Value *bvhLevel, Value *traceRayCtrl, bool isRayQuery, const Twine &PayloadName) {
  Value *stackId = nullptr;

  if (isRayQuery) {
    // For RayQuery - set stack ID to zero since hardware will not use this field
    stackId = getInt32(0);
    if (Ctx.platform.hasEfficient64bEnabled()) {
      stackId = getSyncStackID();
    }
  }

  Value *traceRayCtrlVal = CreateShl(traceRayCtrl, getInt32((uint8_t)TraceRayPayload::PayloadOffsets::traceRayCtrl));
  Value *stackIDVal = CreateShl(stackId, getInt32((uint8_t)TraceRayPayload::PayloadOffsets::stackID));
  Value *bvhLevelVal = CreateShl(bvhLevel, getInt32((uint8_t)TraceRayPayload::PayloadOffsets::bvhLevel));

  Value *payload = CreateOr(stackIDVal, traceRayCtrlVal, PayloadName);
  payload = CreateOr(bvhLevelVal, payload, PayloadName);

  return payload;
}

GenIntrinsicInst *RTBuilder::getSr0_0() {
  Module *module = this->GetInsertBlock()->getModule();

  auto *sr0_0 = this->CreateCall(GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_getSR0_0), {},
                                 VALUE_NAME("sr0.0"));
  return cast<GenIntrinsicInst>(sr0_0);
}

Value *RTBuilder::emitStateRegID(uint32_t BitStart, uint32_t BitEnd) {
  Value *sr0_0 = getSr0_0();

  uint32_t and_imm = BITMASK_RANGE(BitStart, BitEnd);
  uint32_t shr_imm = BitStart;

  Value *andInst = this->CreateAnd(sr0_0, this->getInt32(and_imm), VALUE_NAME("andInst"));

  Value *shrInst = this->CreateLShr(andInst, this->getInt32(shr_imm), VALUE_NAME("shrInst"));
  return shrInst;
}

std::pair<uint32_t, uint32_t> RTBuilder::getSliceIDBitsInSR0() const {
  if (Ctx.platform.GetPlatformFamily() == IGFX_GEN8_CORE || Ctx.platform.GetPlatformFamily() == IGFX_GEN9_CORE) {
    return {14, 15};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_GEN12_CORE ||
             Ctx.platform.GetPlatformFamily() == IGFX_XE_HP_CORE ||
             Ctx.platform.GetPlatformFamily() == IGFX_XE_HPG_CORE) {
    return {11, 13};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_XE_HPC_CORE) {
    return {12, 14};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_XE2_HPG_CORE) {
    return {11, 15};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_XE3_CORE) {
    return {14, 17};
  } else if (Ctx.platform.GetPlatformFamily() >= IGFX_XE3P_CORE) {
    return {14, 17};
  } else {
    return {12, 14};
  }
}

std::pair<uint32_t, uint32_t> RTBuilder::getSubsliceIDBitsInSR0() const {
  if (Ctx.platform.GetPlatformFamily() == IGFX_GEN8_CORE || Ctx.platform.GetPlatformFamily() == IGFX_GEN9_CORE) {
    return {12, 13};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_XE2_HPG_CORE) {
    return {8, 9};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_XE3_CORE) {
    return {8, 11};
  } else if (Ctx.platform.GetPlatformFamily() >= IGFX_XE3P_CORE) {
    return {8, 11};
  } else {
    return {8, 8};
  }
}

std::pair<uint32_t, uint32_t> RTBuilder::getDualSubsliceIDBitsInSR0() const {
  if (Ctx.platform.GetPlatformFamily() == IGFX_GEN11_CORE || Ctx.platform.GetPlatformFamily() == IGFX_GEN11LP_CORE ||
      Ctx.platform.GetPlatformFamily() == IGFX_GEN12LP_CORE || Ctx.platform.GetPlatformFamily() == IGFX_XE_HPC_CORE) {
    return {9, 11};
  } else if (Ctx.platform.GetPlatformFamily() == IGFX_GEN12_CORE ||
             Ctx.platform.GetPlatformFamily() == IGFX_XE_HP_CORE ||
             Ctx.platform.GetPlatformFamily() == IGFX_XE_HPG_CORE) {
    return {9, 10};
  } else {
    IGC_ASSERT_MESSAGE(0, "No support for Dual Subslice in current platform");
    return {0, 0};
  }
}


// globalDSSID is the combined value of sliceID and dssID on slice.
Value *RTBuilder::getGlobalDSSID() {
  if (Ctx.platform.isCoreChildOf(IGFX_XE2_HPG_CORE)) {
    if (Ctx.platform.supportsWMTPForShaderType(Ctx.type)) {
      Module *module = GetInsertBlock()->getModule();
      return CreateCall(GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_logical_subslice_id), {},
                        VALUE_NAME("logical_subslice_id"));
    } else {
      auto dssIDBits = getSubsliceIDBitsInSR0();
      auto sliceIDBits = getSliceIDBitsInSR0();

      if (dssIDBits.first < sliceIDBits.first && sliceIDBits.first == dssIDBits.second + 1) {
        return emitStateRegID(dssIDBits.first, sliceIDBits.second);
      } else if (Ctx.platform.isCoreChildOf(IGFX_XE3_CORE)) {
        Value *sliceID = emitStateRegID(sliceIDBits.first, sliceIDBits.second);
        Value *dssID = emitStateRegID(dssIDBits.first, dssIDBits.second);
        Value *globalDSSID = CreateMul(sliceID, getInt32(NumDSSPerSlice));
        return CreateAdd(globalDSSID, dssID);
      } else {
        Value *dssID = emitStateRegID(dssIDBits.first, dssIDBits.second);
        Value *sliceID = emitStateRegID(sliceIDBits.first, sliceIDBits.second);
        unsigned shiftAmount = dssIDBits.second - dssIDBits.first + 1;
        Value *globalDSSID = CreateShl(sliceID, shiftAmount);
        return CreateOr(globalDSSID, dssID);
      }
    }
  } else {
    auto dssIDBits = getDualSubsliceIDBitsInSR0();
    auto sliceIDBits = getSliceIDBitsInSR0();

    if (dssIDBits.first < sliceIDBits.first && sliceIDBits.first == dssIDBits.second + 1) {
      return emitStateRegID(dssIDBits.first, sliceIDBits.second);
    } else {
      Value *dssID = emitStateRegID(dssIDBits.first, dssIDBits.second);
      Value *sliceID = emitStateRegID(sliceIDBits.first, sliceIDBits.second);
      unsigned shiftAmount = dssIDBits.second - dssIDBits.first + 1;
      Value *globalDSSID = CreateShl(sliceID, shiftAmount);
      return CreateOr(globalDSSID, dssID);
    }
  }
}


Value *RTBuilder::getRootNodePtr(Value *BVHPtr) {
  if (IGC_IS_FLAG_ENABLED(ForceNullBVH)) {
    // Force all BVHs to be null. Infinitely fast ray traversal.
    return this->getInt64(0);
  }

  Value *BVHI = this->CreatePointerCast(BVHPtr, this->getInt64Ty(), VALUE_NAME("&BVH"));

  return _getBVHPtr(BVHI, getInt64(Ctx.bvhInfo.hasFixedOffset ? Ctx.bvhInfo.offset : 0),
                    getInt1(Ctx.bvhInfo.hasFixedOffset), VALUE_NAME("rootNodePtr"));
}

void RTBuilder::setHitT(StackPointerVal *StackPointer, Value *V, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _setHitT_##X(StackPointer, V, VAdapt{*this, Committed});                                                           \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}

Value *RTBuilder::getHitT(StackPointerVal *StackPointer, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getHitT_##X(StackPointer, VAdapt{*this, Committed},                                                        \
                        VALUE_NAME(Committed ? "committedHit.t" : "potentialHit.t"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::isDoneBitNotSet(StackPointerVal *StackPointer, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _isDoneBitNotSet_##X(StackPointer, VAdapt{*this, Committed}, VALUE_NAME("!done"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getBvhLevel(StackPointerVal *StackPointer, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getBvhLevel_##X(StackPointer, VAdapt{*this, Committed});
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getHitValid(RTBuilder::StackPointerVal *StackPointer, Value *CommittedHit) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _isValid_##X(StackPointer, CommittedHit, VALUE_NAME("valid"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

void RTBuilder::setHitValid(StackPointerVal *StackPointer, bool CommittedHit) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _setHitValid_##X(StackPointer, VAdapt{*this, CommittedHit});                                                       \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}

LoadInst *RTBuilder::getSyncTraceRayControl(GetElementPtrInst *ptrCtrl) {
  return this->CreateLoad(ptrCtrl->getResultElementType(), ptrCtrl, VALUE_NAME("rayQueryObject.traceRayControl"));
}

void RTBuilder::setSyncTraceRayControl(GetElementPtrInst *ptrCtrl, RTStackFormat::TraceRayCtrl ctrl) {
  this->CreateStore(llvm::ConstantInt::get(ptrCtrl->getResultElementType(), (uint32_t)ctrl), ptrCtrl);
}

Value *RTBuilder::getHitBaryCentric(RTBuilder::StackPointerVal *StackPointer, uint32_t idx, Value *CommittedHit) {
  IGC_ASSERT_MESSAGE(idx == 0 || idx == 1, "Only U V are supported.");
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getHitBaryCentric_##X(StackPointer, VAdapt{*this, idx}, CommittedHit,                                      \
                                  VALUE_NAME(idx ? "MemHit.v" : "MemHit.u"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}


Value *RTBuilder::getLeafType(StackPointerVal *StackPointer, Value *CommittedHit) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _createLeafType_##X(StackPointer, CommittedHit, VALUE_NAME("leafType"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}



Value *RTBuilder::getLeafNodeSubType(StackPointerVal *StackPointer, Value *CommittedHit) {
  switch (getMemoryStyle()) {

  case RTMemoryStyle::Xe:
    return this->getInt32(0);

#define STYLE_XE3PLUS(X)                                                                                               \
  case RTMemoryStyle::X:                                                                                               \
    return _getLeafNodeSubType_##X(StackPointer, CommittedHit, VALUE_NAME("MemHit.LeafNodeSubType"));

#include "RayTracingMemoryStyleXe3Plus.h"
#undef STYLE_XE3PLUS
  }

  IGC_ASSERT(0);

  return nullptr;
}


CallInst *RTBuilder::CreateLSCFence(LSC_SFID SFID, LSC_SCOPE Scope, LSC_FENCE_OP FenceOp) {
  Function *pFunc =
      GenISAIntrinsic::getDeclaration(this->GetInsertBlock()->getModule(), GenISAIntrinsic::GenISA_LSCFence);

  Value *VSFID = this->getInt32(SFID);
  Value *VScope = this->getInt32(Scope);
  Value *VOp = this->getInt32(FenceOp);

  Value *Args[] = {VSFID, VScope, VOp};

  return this->CreateCall(pFunc, Args);
}

Value *RTBuilder::canonizePointer(Value *Ptr) {
  // A64 addressing bounds checking requires the address to be canonical
  // which means:
  //
  // addr[63:VA_MSB+1] is same as addr[VA_MSB]
  //
  // For DG2, MSB for the virtual address (i.e., VA_MSB) == 47.
  //
  // We must sign extend this bit explicitly in the shader for addresses
  // that we create.  Anything coming from the UMD should already be
  // canonized.

  uint32_t VA_MSB = 0;
  switch (getMemoryStyle()) {
  case RTMemoryStyle::Xe:
    VA_MSB = 48 - 1;
    break;
  case RTMemoryStyle::Xe3:
  case RTMemoryStyle::Xe3PEff64:
    VA_MSB = 57 - 1;
    break;
  }
  constexpr uint32_t MSB = 63;
  uint32_t ShiftAmt = MSB - VA_MSB;
  auto *P2I = this->CreateBitOrPointerCast(Ptr, this->getInt64Ty());
  auto *Canonized = this->CreateShl(P2I, ShiftAmt);
  Canonized = this->CreateAShr(Canonized, ShiftAmt);
  Canonized = this->CreateBitOrPointerCast(Canonized, Ptr->getType(), VALUE_NAME(Ptr->getName() + Twine("-canonized")));

  return Canonized;
}


void RTBuilder::setReturnAlignment(CallInst *CI, uint32_t AlignVal) {
  Align Alt(AlignVal);
  CI->addRetAttr(Attribute::getWithAlignment(CI->getContext(), Alt));
}

void RTBuilder::setDereferenceable(CallInst *CI, uint32_t Size) {
  if (Size) {
    CI->addRetAttr(Attribute::getWithDereferenceableBytes(CI->getContext(), Size));
  }
}

Value *RTBuilder::getGlobalBufferPtr(IGC::ADDRESS_SPACE Addrspace) {
  // If explicit GlobalBuffer pointer is set, use it instead
  // of getting it via intrinsic.
  // This might be the case on e.g. OpenCL path.
  if (this->GlobalBufferPtr) {
    return this->GlobalBufferPtr;
  }

  auto *M = this->GetInsertBlock()->getModule();

  auto *PtrTy = this->getRayDispatchGlobalDataPtrTy(*M, Addrspace);

  Function *Func = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_GlobalBufferPointer, PtrTy);

  CallInst *CI = this->CreateCall(Func, {}, VALUE_NAME("globalPtr"));

  if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
    this->setReturnAlignment(CI, RTGlobalsAlign);

  return CI;
}

Value *RTBuilder::getGlobalBufferPtrForSlot(IGC::ADDRESS_SPACE Addrspace, Value *slot) {
  auto *pFunc =
      GenISAIntrinsic::getDeclaration(Ctx.getModule(), GenISAIntrinsic::GenISA_RuntimeValue,
                                      getRayDispatchGlobalDataPtrTy(*Ctx.getModule(), ADDRESS_SPACE_CONSTANT));

  auto *mainGlobalBufferPtr = CreateCall(pFunc, getInt32(Ctx.getModuleMetaData()->pushInfo.inlineRTGlobalPtrOffset),
                                         VALUE_NAME("globalBufferPtrFromRuntimeValue"));

  auto *offset = CreateMul(slot, getInt32(IGC::Align(sizeof(RayDispatchGlobalData), IGC::RTGlobalsAlign)));

  auto *globalBufferPtr = CreateBitCast(mainGlobalBufferPtr, getInt8PtrTy(ADDRESS_SPACE_CONSTANT));
  globalBufferPtr = CreateInBoundsGEP(getInt8Ty(), globalBufferPtr, offset);
  globalBufferPtr = CreateBitCast(globalBufferPtr, mainGlobalBufferPtr->getType(), VALUE_NAME("globalBuffer[]"));

  return globalBufferPtr;
}


Value *RTBuilder::getSyncStackID() {
  auto &PlatformInfo = Ctx.platform.getPlatformInfo();

  if (PlatformInfo.eProductFamily == IGFX_DG2 || PlatformInfo.eProductFamily == IGFX_ARROWLAKE ||
      PlatformInfo.eProductFamily == IGFX_METEORLAKE) {
    return _getSyncStackID_Xe(VALUE_NAME("SyncStackID"));
  } else if (PlatformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE) {
    return _getSyncStackID_Xe_HPC(VALUE_NAME("SyncStackID"));
  } else if (PlatformInfo.eProductFamily == IGFX_BMG || PlatformInfo.eProductFamily == IGFX_LUNARLAKE) {
    return _getSyncStackID_Xe2(VALUE_NAME("SyncStackID"));
  } else if (PlatformInfo.eRenderCoreFamily == IGFX_XE3_CORE) {
    return _getSyncStackID_Xe3(VALUE_NAME("SyncStackID"));
  } else if (PlatformInfo.eRenderCoreFamily >= IGFX_XE3P_CORE) {
    if (Ctx.platform.hasEfficient64bEnabled() && IGC_IS_FLAG_DISABLED(DisableSWManagedStack)) {
      return _getSyncStackID_Xe3pEff64(VALUE_NAME("SyncStackID"));
    } else {
      return _getSyncStackID_Xe3p(VALUE_NAME("SyncStackID"));
    }
  } else {
    IGC_ASSERT_MESSAGE(0, "Invalid Product Family for SyncStackID");
  }

  return {};
}

bool RTBuilder::checkAlign(Module &M, StructType *StructTy, uint32_t Align) {
  auto &DL = M.getDataLayout();
  auto Layout = DL.getStructLayout(StructTy);

  uint64_t Size = Layout->getSizeInBytes();
  uint64_t Diff = Size % Align;
  return (Diff == 0);
}


uint32_t RTBuilder::getSWStackStatefulAddrSpace(const ModuleMetaData &MMD) {
  auto &rtInfo = MMD.rtInfo;
  IGC_ASSERT_MESSAGE(rtInfo.SWStackAddrspace != UINT_MAX, "not initialized?");
  return rtInfo.SWStackAddrspace;
}

uint32_t RTBuilder::getRTSyncStackStatefulAddrSpace(const ModuleMetaData &MMD) {
  auto &rtInfo = MMD.rtInfo;
  IGC_ASSERT_MESSAGE(rtInfo.RTSyncStackAddrspace != UINT_MAX, "not initialized?");
  return rtInfo.RTSyncStackAddrspace;
}


static const char *RaytracingTypesMDName = "igc.magic.raytracing.types";

// If you need to add another type to the metadata, add a slot for it here
// and add create a getter function below to retrieve it.
enum class RaytracingType {
  AsyncRTStack2Stateless = 0,
  AsyncRTStack2Stateful = 1,
  RayDispatchGlobalData = 2,
  SWHotZone = 3,
  SyncRTStack2Stateless = 4,
  SyncRTStack2Stateful = 5,
  NUM_TYPES
};

// Initialize the metadata with the number of types marked as undef.  These
// will later be updated to null values with the actual types.
NamedMDNode *initTypeMD(Module &M, uint32_t NumEntries) {
  auto *TypesMD = M.getOrInsertNamedMetadata(RaytracingTypesMDName);
  auto *Val = UndefValue::get(Type::getInt8PtrTy(M.getContext()));
  auto *Node = MDNode::get(M.getContext(), ConstantAsMetadata::get(Val));

  for (uint32_t i = 0; i < NumEntries; i++)
    TypesMD->addOperand(Node);

  return TypesMD;
}

static Type *setRTTypeMD(Module &M, RaytracingType Idx, NamedMDNode *TypesMD, Type *Ty, uint64_t ExpectedSize,
                         uint32_t AddrSpace) {
  IGC_ASSERT_MESSAGE(ExpectedSize == M.getDataLayout().getTypeAllocSize(Ty), "mismatch?");
  // Replace the undef value as set from initTypeMD() with a null pointer
  // to indicate that we have the type in place.
  auto *Val = ConstantPointerNull::get(Ty->getPointerTo(AddrSpace));
  auto *Node = MDNode::get(M.getContext(), ConstantAsMetadata::get(Val));
  TypesMD->setOperand((uint32_t)Idx, Node);
  return Val->getType();
}

static Type *lazyGetRTType(Module &M, RaytracingType Idx,
                           std::function<Type *(NamedMDNode *, RaytracingType)> ForceFn) {
  auto *TypesMD = M.getNamedMetadata(RaytracingTypesMDName);
  if (!TypesMD)
    TypesMD = initTypeMD(M, (uint32_t)RaytracingType::NUM_TYPES);

  auto *TypesNode = TypesMD->getOperand((uint32_t)Idx);

  auto *MD = TypesNode->getOperand(0).get();
  auto *C = cast<ConstantAsMetadata>(MD);
  if (isa<UndefValue>(C->getValue())) {
    return ForceFn(TypesMD, Idx);
  }

  return C->getValue()->getType();
}

Type *RTBuilder::getSMStack2Ty() const {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _gettype_SMStack2_##X(*Ctx.getModule());
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Type *RTBuilder::getRTStack2Ty() const {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _gettype_RTStack2_##X(*Ctx.getModule());
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

Type *RTBuilder::getRTStack2PtrTy(bool async) const {
  auto &rtInfo = Ctx.getModuleMetaData()->rtInfo;
  auto &SSO = async ? rtInfo.RTAsyncStackSurfaceStateOffset : rtInfo.RTSyncStackSurfaceStateOffset;
  return getRTStack2PtrTy(SSO ? RTBuilder::STATEFUL : RTBuilder::STATELESS, async);
}

Type *RTBuilder::getRTStack2PtrTy(RTBuilder::RTMemoryAccessMode Mode, bool async) const {
  IGC_ASSERT_MESSAGE((Mode == RTBuilder::STATELESS || Mode == RTBuilder::STATEFUL), "unknown?");

  auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
    uint32_t AddrSpace = ADDRESS_SPACE_NUM_ADDRESSES;
    if (Mode == RTBuilder::STATELESS) {
      AddrSpace = ADDRESS_SPACE_GLOBAL;
    } else {
      {
        AddrSpace = getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData());
      }
    }
    IGC_ASSERT(AddrSpace != ADDRESS_SPACE_NUM_ADDRESSES);

    uint32_t RTStackHeaderSize = RTStackFormat::getRTStackHeaderSize(MAX_BVH_LEVELS);
    // In new stack layout, the logical stack size is the same as in "legacy" layout.
    // However, its representation in memory differs - unused space must be taken into account - see doc,
    // and this function is used for memory access implementation, hence:
    if (getMemoryStyle() == RTMemoryStyle::Xe3PEff64) {
      RTStackHeaderSize = sizeof(RTStack<Xe3PEff64, MAX_BVH_LEVELS>);
    }

    return setRTTypeMD(*Ctx.getModule(), Idx, TypesMD, getRTStack2Ty(), RTStackHeaderSize, AddrSpace);
  };

  auto RTType = RaytracingType::NUM_TYPES;

  if (async) {
    RTType =
        (Mode == RTBuilder::STATELESS) ? RaytracingType::AsyncRTStack2Stateless : RaytracingType::AsyncRTStack2Stateful;
  } else {
    RTType =
        (Mode == RTBuilder::STATELESS) ? RaytracingType::SyncRTStack2Stateless : RaytracingType::SyncRTStack2Stateful;
  }

  return lazyGetRTType(*Ctx.getModule(), RTType, addTy);
}

Type *RTBuilder::getRayDispatchGlobalDataPtrTy(Module &M, IGC::ADDRESS_SPACE Addrspace) {
  auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
    auto *Ty = _gettype_RayDispatchGlobalData(M);
    return setRTTypeMD(M, Idx, TypesMD, Ty, sizeof(RayDispatchGlobalData), Addrspace);
  };

  return lazyGetRTType(M, RaytracingType::RayDispatchGlobalData, addTy);
}


// Find the Insert point which is placed
// after all Allocas and after all the Instructions
// from the optional vector additionalInstructionsToSkip.
Instruction *RTBuilder::getEntryFirstInsertionPt(Function &F,
                                                 const std::vector<Value *> *additionalInstructionsToSkip) {
  auto &EntryBB = F.getEntryBlock();

  // The insert point will be right after the last alloca
  // assumes a well-formed block with a terminator at the end
  auto *CurIP = &*EntryBB.begin();
  for (auto &I : EntryBB) {
    bool skipInstruction = false;

    if (additionalInstructionsToSkip != nullptr) {
      for (const Value *inst : *additionalInstructionsToSkip) {
        if (inst == &I) {
          skipInstruction = true;
          break;
        }
      }
    }

    if (isa<AllocaInst>(&I) || skipInstruction)
      CurIP = I.getNextNode();
  }

  return CurIP;
}

Type *RTBuilder::getInt64PtrTy(unsigned int AddrSpace) const { return Type::getInt64PtrTy(this->Context, AddrSpace); }

Type *RTBuilder::getInt32PtrTy(unsigned int AddrSpace) const { return Type::getInt32PtrTy(this->Context, AddrSpace); }

IGC::RTMemoryStyle RTBuilder::getMemoryStyle() const { return Ctx.getModuleMetaData()->rtInfo.MemStyle; }


SpillValueIntrinsic *RTBuilder::getSpillValue(Value *Val, uint64_t Offset) {
  auto *M = this->GetInsertBlock()->getModule();
  Function *SpillFunction = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_SpillValue, Val->getType());
  // create spill instruction
  Value *Args[] = {Val, this->getInt64(Offset)};
  auto *SpillValue = this->CreateCall(SpillFunction, Args);

  return cast<SpillValueIntrinsic>(SpillValue);
}

FillValueIntrinsic *RTBuilder::getFillValue(Type *Ty, uint64_t Offset, const Twine &Name) {
  auto *M = this->GetInsertBlock()->getModule();
  Function *FillFunction = GenISAIntrinsic::getDeclaration(M, GenISAIntrinsic::GenISA_FillValue, Ty);
  auto *FillValue = this->CreateCall(FillFunction, this->getInt64(Offset), Name);

  return cast<FillValueIntrinsic>(FillValue);
}

void RTBuilder::setGlobalBufferPtr(Value *GlobalBufferPtr) { this->GlobalBufferPtr = GlobalBufferPtr; }

void RTBuilder::setDisableRTGlobalsKnownValues(bool Disable) { this->DisableRTGlobalsKnownValues = Disable; }

ConstantInt *RTBuilder::supportStochasticLod() { return getInt1(Ctx.platform.supportStochasticLod()); }

ConstantInt *RTBuilder::isRayQueryReturnOptimizationEnabled() {
  return getInt1(Ctx.platform.isRayQueryReturnOptimizationEnabled());
}


GenIntrinsicInst *RTBuilder::createDummyInstID(Value *pSrcVal) {
  Module *module = GetInsertBlock()->getModule();
  Function *pFunc = GenISAIntrinsic::getDeclaration(module, GenISAIntrinsic::GenISA_dummyInstID, pSrcVal->getType());
  auto *CI = CreateCall(pFunc, pSrcVal);
  return cast<GenIntrinsicInst>(CI);
}

CallInst *RTBuilder::ctlz(Value *V) {
  auto *Ctlz = Intrinsic::getDeclaration(GetInsertBlock()->getModule(), Intrinsic::ctlz, V->getType());
  return CreateCall2(Ctlz, V, getFalse(), VALUE_NAME("lzd"));
}

CallInst *RTBuilder::cttz(Value *V) {
  auto *Cttz = Intrinsic::getDeclaration(GetInsertBlock()->getModule(), Intrinsic::cttz, V->getType());
  return CreateCall2(Cttz, V, getFalse(), VALUE_NAME("cttz"));
}

void RTBuilder::createPotentialHit2CommittedHit(StackPointerVal *StackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _createPotentialHit2CommittedHit_##X(StackPtr);                                                                    \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}

void RTBuilder::createTraceRayInlinePrologue(StackPointerVal *StackPtr, Value *RayInfo, Value *RootNodePtr,
                                             Value *RayFlags, Value *InstanceInclusionMask, Value *ComparisonValue,
                                             Value *TMax, bool updateFlags, bool initialDoneBitValue) {
  switch (getMemoryStyle()) {
  case RTMemoryStyle::Xe:
    _createTraceRayInlinePrologue_Xe(StackPtr, RayInfo, RootNodePtr, RayFlags, InstanceInclusionMask, ComparisonValue,
                                     TMax, VAdapt{*this, updateFlags}, VAdapt{*this, initialDoneBitValue});
    break;
  case RTMemoryStyle::Xe3:
    _createTraceRayInlinePrologue_Xe3(StackPtr, RayInfo, RootNodePtr, RayFlags, InstanceInclusionMask, ComparisonValue,
                                      TMax, VAdapt{*this, updateFlags}, VAdapt{*this, initialDoneBitValue});
    break;
  case RTMemoryStyle::Xe3PEff64:
    _createTraceRayInlinePrologue_Xe3PEff64(StackPtr, RayInfo, RootNodePtr, RayFlags, InstanceInclusionMask,
                                            ComparisonValue, TMax, VAdapt{*this, updateFlags},
                                            VAdapt{*this, initialDoneBitValue});
    break;
  }
}

void RTBuilder::emitSingleRQMemRayWrite(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr,
                                        bool singleRQProceed) {
  switch (getMemoryStyle()) {
  case RTMemoryStyle::Xe:
    _emitSingleRQMemRayWrite_Xe(HWStackPtr, SMStackPtr, VAdapt{*this, singleRQProceed});
    break;

#define STYLE_XE3PLUS(X)                                                                                               \
  case RTMemoryStyle::X:                                                                                               \
    _emitSingleRQMemRayWrite_##X(HWStackPtr, SMStackPtr);                                                              \
    break;

#include "RayTracingMemoryStyleXe3Plus.h"
#undef STYLE_XE3PLUS
  }
}

void RTBuilder::copyMemHitInProceed(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr,
                                    bool singleRQProceed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _copyMemHitInProceed_##X(HWStackPtr, SMStackPtr, VAdapt{*this, singleRQProceed});                                  \
    break;

#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}

Value *RTBuilder::syncStackToShadowMemory(SyncStackPointerVal *HWStackPtr, SyncStackPointerVal *SMStackPtr,
                                          Value *ProceedReturnVal, Value *ShadowMemRTCtrlPtr) {

  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _syncStackToShadowMemory_##X(HWStackPtr, SMStackPtr, ProceedReturnVal, ShadowMemRTCtrlPtr);
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }

  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getCommittedStatus(SyncStackPointerVal *SMStackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getCommittedStatus_##X(SMStackPtr);
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }

  IGC_ASSERT(0);
  return {};
}

Value *RTBuilder::getCandidateType(SyncStackPointerVal *SMStackPtr) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getCandidateType_##X(SMStackPtr, VALUE_NAME("CandidateType"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }

  IGC_ASSERT(0);
  return {};
}

void RTBuilder::commitProceduralPrimitiveHit(SyncStackPointerVal *SMStackPtr, Value *THit) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    _commitProceduralPrimitiveHit_##X(SMStackPtr, THit);                                                               \
    break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
}

Value *RTBuilder::getHitAddress(StackPointerVal *StackPtr, bool Committed) {
  switch (getMemoryStyle()) {
#define STYLE(X)                                                                                                       \
  case RTMemoryStyle::X:                                                                                               \
    return _getHitAddress_##X(StackPtr, VAdapt{*this, Committed});
#include "RayTracingMemoryStyle.h"
#undef STYLE
  }
  IGC_ASSERT(0);
  return {};
}

template <typename StackPointerValT, typename RayInfoIntrinsicT>
Value *RTBuilder::lowerRayInfo(StackPointerValT *perLaneStackPtr, RayInfoIntrinsicT *I, Value *shaderType,
                               Value *isProcedural) {
  bool checkInstanceLeafPtr = std::is_same_v<RayInfoIntrinsicT, RayQueryInfoIntrinsic>;

  Value *info = nullptr;
  uint32_t dim = I->getDim();
  auto infoKind = I->getInfoKind();

  switch (infoKind) {
  case WORLD_RAY_ORG:
    info = getWorldRayOrig(perLaneStackPtr, dim);
    break;
  case WORLD_RAY_DIR:
    info = getWorldRayDir(perLaneStackPtr, dim);
    break;
  case OBJ_RAY_ORG:
    info = getObjRayOrig(perLaneStackPtr, dim, shaderType, I, checkInstanceLeafPtr);
    break;
  case OBJ_RAY_DIR:
    info = getObjRayDir(perLaneStackPtr, dim, shaderType, I, checkInstanceLeafPtr);
    break;
  case RAY_T_MIN:
    info = getRayTMin(perLaneStackPtr);
    break;
  case RAY_T_CURRENT:
    info = getRayTCurrent(perLaneStackPtr, shaderType);
    break;
  case INSTANCE_ID:
    info = getInstanceID(perLaneStackPtr, shaderType, I, checkInstanceLeafPtr);
    break;
  case INSTANCE_INDEX:
    info = getInstanceIndex(perLaneStackPtr, shaderType, I, checkInstanceLeafPtr);
    break;
  case PRIMITIVE_INDEX: {
    auto *nodeType = isProcedural ? CreateSelect(isProcedural, getInt32(NODE_TYPE_PROCEDURAL), getInt32(NODE_TYPE_QUAD))
                                  : getLeafType(perLaneStackPtr,
                                                CreateICmpEQ(shaderType, getInt32(CallableShaderTypeMD::ClosestHit)));
    info = getPrimitiveIndex(perLaneStackPtr, I, nodeType, shaderType, checkInstanceLeafPtr);
    break;
  }
  case RAY_FLAGS:
    info = CreateZExt(getRayFlags(perLaneStackPtr), I->getType());
    break;
  case OBJECT_TO_WORLD:
    info = getObjToWorld(perLaneStackPtr, dim, shaderType, I, checkInstanceLeafPtr);
    break;
  case WORLD_TO_OBJECT:
    info = getWorldToObj(perLaneStackPtr, dim, shaderType, I, checkInstanceLeafPtr);
    break;
  case GEOMETRY_INDEX: {
    auto *nodeType = isProcedural ? CreateSelect(isProcedural, getInt32(NODE_TYPE_PROCEDURAL), getInt32(NODE_TYPE_QUAD))
                                  : getLeafType(perLaneStackPtr,
                                                CreateICmpEQ(shaderType, getInt32(CallableShaderTypeMD::ClosestHit)));
    info = getGeometryIndex(perLaneStackPtr, I, nodeType, shaderType, checkInstanceLeafPtr);
    break;
  }
  case INST_CONTRIBUTION_TO_HITGROUP_INDEX:
    info = getInstanceContributionToHitGroupIndex(perLaneStackPtr, shaderType);
    break;
  case RAY_MASK:
    info = getRayMask(perLaneStackPtr);
    break;
  case TRIANGLE_FRONT_FACE:
  case CANDIDATE_PROCEDURAL_PRIM_NON_OPAQUE: // Procedural Primitive Opaque Info is stored in Front Face bit
  {
    info = getIsFrontFace(perLaneStackPtr, shaderType);

    if (infoKind == CANDIDATE_PROCEDURAL_PRIM_NON_OPAQUE)
      info = CreateICmpEQ(info, getInt1(0), VALUE_NAME("is_nonopaque"));

    break;
  }
  case BARYCENTRICS: {
    info = getHitBaryCentric(perLaneStackPtr, I->getDim(),
                             CreateICmpEQ(shaderType, getInt32(CallableShaderTypeMD::ClosestHit)));
    break;
  }
  default:
    IGC_ASSERT_MESSAGE(0, "Unsupported Ray Info");
    break;
  }

  return info;
}
template Value *RTBuilder::lowerRayInfo<RTBuilder::SyncStackPointerVal, RayQueryInfoIntrinsic>(
    RTBuilder::SyncStackPointerVal *, RayQueryInfoIntrinsic *, Value *, std::optional<bool>);


