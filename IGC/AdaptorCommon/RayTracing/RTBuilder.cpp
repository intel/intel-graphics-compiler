/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A subclass of IRBuilder that provides methods for raytracing functionality.
///
//===----------------------------------------------------------------------===//

#include "IGC/common/StringMacros.hpp"
#include "RTBuilder.h"
#include "RTArgs.h"
#include "iStdLib/utility.h"
#include "common/debug/DebugMacros.hpp"
#include "common/MDFrameWork.h"
#include "common/igc_regkeys.hpp"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "RTStackFormat.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/Attributes.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace RTStackFormat;
using namespace IGC;

namespace {
    class VAdapt
    {
        Value* VA = nullptr;
    public:
        explicit VAdapt(std::nullptr_t) = delete;
        explicit VAdapt(Value* V)                      : VA(V) {}
        explicit VAdapt(IRBuilder<>& IRB, uint32_t V)  : VA(IRB.getInt32(V)) {}
        explicit VAdapt(IRBuilder<>& IRB, uint64_t V)  : VA(IRB.getInt64(V)) {}
        explicit VAdapt(IRBuilder<>& IRB, bool V)      : VA(IRB.getInt1(V)) {}
        explicit VAdapt(IRBuilder<>& IRB, IGC::CallableShaderTypeMD V) : VA(IRB.getInt32(V)) {}
        operator Value*() const { return VA; }
    };
};

std::pair<BasicBlock*, BasicBlock*>
RTBuilder::createTriangleFlow(
    Value* Cond,
    Instruction* InsertPoint,
    const Twine &TrueBBName,
    const Twine &JoinBBName)
{
    auto& C = InsertPoint->getContext();
    auto* StartBB = InsertPoint->getParent();
    auto* JoinBB = StartBB->splitBasicBlock(
        InsertPoint,
        JoinBBName);

    auto* F = InsertPoint->getFunction();
    auto* TrueBB = BasicBlock::Create(C, TrueBBName, F, JoinBB);
    this->SetInsertPoint(TrueBB);
    this->CreateBr(JoinBB);

    StartBB->getTerminator()->eraseFromParent();
    this->SetInsertPoint(StartBB);
    this->CreateCondBr(Cond, TrueBB, JoinBB);

    return std::make_pair(TrueBB, JoinBB);
}

void RTBuilder::setInvariantLoad(LoadInst* LI)
{
    auto *EmptyNode = MDNode::get(LI->getContext(), nullptr);
    if (IGC_IS_FLAG_DISABLED(DisableInvariantLoad))
        LI->setMetadata(LLVMContext::MD_invariant_load, EmptyNode);
}

Value* RTBuilder::getRtMemBasePtr(void)
{
#define STYLE(X)                                                 \
    static_assert(                                               \
        offsetof(RayDispatchGlobalData::RT::Xe, rtMemBasePtr) == \
        offsetof(RayDispatchGlobalData::RT::X,  rtMemBasePtr));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    return _get_rtMemBasePtr_Xe(VALUE_NAME("rtMemBasePtr"));
}


Value* RTBuilder::getMaxBVHLevels(void)
{
    if (!DisableRTGlobalsKnownValues)
        return this->getInt32(MAX_BVH_LEVELS);

    switch (getMemoryStyle())
    {
    case RTMemoryStyle::Xe:
        return _get_maxBVHLevels_Xe(VALUE_NAME("maxBVHLevels"));
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getStatelessScratchPtr(void)
{
    return _get_statelessScratchPtr(VALUE_NAME("statelessScratchPtr"));
}


Value* RTBuilder::getIsFrontFace(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                     \
    case RTMemoryStyle::X:                           \
        return _getIsFrontFace_##X(                  \
            StackPointer,                            \
            VAdapt{ *this, ShaderTy == ClosestHit }, \
            VALUE_NAME("is_front_face"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}


Value* RTBuilder::CreateSyncStackPtrIntrinsic(
    Value* Addr, Type* PtrTy, bool AddDecoration)
{
    Module* M = this->GetInsertBlock()->getModule();
    Type* Tys[] = { PtrTy, Addr->getType() };
    CallInst* StackPtr = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M, GenISAIntrinsic::GenISA_SyncStackPtr, Tys),
        Addr,
        VALUE_NAME("perLaneSyncStackPointer"));

    if (AddDecoration)
    {
        this->setReturnAlignment(StackPtr, RTStackAlign);
        if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        {
            this->setDereferenceable(StackPtr, getRTStack2Size());
        }
    }

    return StackPtr;
}



Value* RTBuilder::getGlobalSyncStackID()
{
    // global sync stack id = dssIDGlobal * NUM_SIMD_LANES_PER_DSS + SyncStackID
    Value* IDGlobal = this->getGlobalDSSID();
    Value* numSimdLanesPerDss = this->getInt32(Ctx.platform.getRTStackDSSMultiplier());

    Value* val = this->CreateMul(IDGlobal, numSimdLanesPerDss);

    Value* stackID = this->getSyncStackID();
    val = this->CreateAdd(
        val,
        this->CreateZExt(stackID, this->getInt32Ty()),
        VALUE_NAME("globalSyncStackID"));

    return val;
}

uint32_t RTBuilder::getRTStack2Size() const
{
    switch (getMemoryStyle())
    {
#define STYLE(X) case RTMemoryStyle::X: return sizeof(RTStack2<RTStackFormat::X>);
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return 0;
}

Value* RTBuilder::getRTStackSize(uint32_t Align)
{
#define STYLE(X) static_assert(sizeof(RTStack2<Xe>) == sizeof(RTStack2<RTStackFormat::X>));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    // syncStackSize = sizeof(HitInfo)*2 + (sizeof(Ray) + sizeof(TravStack))*RTDispatchGlobals.maxBVHLevels
    Value* stackSize = this->CreateMul(
        this->getInt32(sizeof(MemRay<Xe>) + sizeof(MemTravStack)),
        this->getMaxBVHLevels());

    stackSize = this->CreateAdd(
        stackSize, this->getInt32(sizeof(MemHit<Xe>) * 2),
        VALUE_NAME("RTStackSize"));

    stackSize = this->alignVal(stackSize, Align);

    return stackSize;
}


Value* RTBuilder::getSyncRTStackSize()
{
    return this->getRTStackSize(RayDispatchGlobalData::StackChunkSize);
}

Value* RTBuilder::getSyncStackOffset(bool rtMemBasePtr)
{
    // Per thread Synchronous RTStack is calculated using the following formula:
    // syncBase = RTDispatchGlobals.rtMemBasePtr - (GlobalSyncStackID + 1) * syncStackSize;
    // If we start from syncStack, then, offset should be:
    // syncBase = syncStack + (NumSyncStackSlots - (GlobalSyncStackID + 1)) * syncStackSize
    // Where:
    Value* globalStackID = this->getGlobalSyncStackID();
    Value* OffsetID = this->CreateAdd(globalStackID, this->getInt32(1));

    if (!rtMemBasePtr)
    {
        OffsetID = this->CreateSub(this->getInt32(getNumSyncStackSlots()), OffsetID);
    }

    Value* stackSize = this->getSyncRTStackSize();

    return this->CreateMul(
        this->CreateZExt(OffsetID, this->getInt32Ty()),
        this->CreateZExt(stackSize, this->getInt32Ty()),
        VALUE_NAME("SyncStackOffset"));
}

RTBuilder::SyncStackPointerVal* RTBuilder::getSyncStackPointer(Value* syncStackOffset, RTBuilder::RTMemoryAccessMode Mode)
{
    auto* PointeeTy = getRTStack2Ty();
    if (Mode == RTBuilder::STATEFUL)
    {
        uint32_t AddrSpace = getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData());
        Value* perLaneStackPointer = this->CreateIntToPtr(syncStackOffset, PointerType::get(PointeeTy, AddrSpace));
        return static_cast<RTBuilder::SyncStackPointerVal*>(perLaneStackPointer);
    }
    else
    {
        Value* memBasePtr = nullptr;
        {
            memBasePtr = this->getRtMemBasePtr();
        }
        IGC_ASSERT(memBasePtr != nullptr);

        Value* stackBase = this->CreatePtrToInt(memBasePtr, this->getInt64Ty());
        Value* perLaneStackPointer = this->CreateSub(
            stackBase,
            this->CreateZExt(syncStackOffset, stackBase->getType()),
            VALUE_NAME("HWMem.perLaneSyncStackPointer"));
        perLaneStackPointer = this->CreateIntToPtr(perLaneStackPointer, PointerType::get(PointeeTy, ADDRESS_SPACE_GLOBAL));
        return static_cast<RTBuilder::SyncStackPointerVal*>(perLaneStackPointer);
    }
}

RTBuilder::SyncStackPointerVal* RTBuilder::getSyncStackPointer()
{
    auto Mode = Ctx.getModuleMetaData()->rtInfo.RTSyncStackSurfaceStateOffset ?
        RTBuilder::STATEFUL :
        RTBuilder::STATELESS;

    // requests for the sync stack pointer in early phases of compilation
    // will return a marker intrinsic that can be analyzed later by cache ctrl pass.
    // "marker" here means this intrinsic will be lowered to almost nothing eventually.
    auto* PtrTy = this->getRTStack2PtrTy(Mode, false);

    Value* stackOffset = this->getSyncStackOffset(RTBuilder::STATELESS == Mode);
    stackOffset = this->getSyncStackPointer(stackOffset, Mode);
    return static_cast<RTBuilder::SyncStackPointerVal*>(
        this->CreateSyncStackPtrIntrinsic(stackOffset, PtrTy, true));
}


// Note: 'traceRayCtrl' should be already by 8 bits to its location
// in the payload before passing as an argument to this function.
// getTraceRayPayload() just ORs together the bvh, ctrl, and stack id.
TraceRayIntrinsic* RTBuilder::createTraceRay(
    Value* bvhLevel,
    Value* traceRayCtrl,
    bool isRayQuery,
    const Twine& PayloadName)
{
    constexpr uint16_t TraceRayCtrlOffset =
        offsetof(TraceRayMessage::Payload, traceRayCtrl) * 8;
    Value* traceRayCtrlVal = this->CreateShl(traceRayCtrl, TraceRayCtrlOffset);
    Module* module = this->GetInsertBlock()->getModule();

    Value* GlobalPointer = this->getGlobalBufferPtr();

    GenISAIntrinsic::ID ID = isRayQuery ?
        GenISAIntrinsic::GenISA_TraceRaySync :
        GenISAIntrinsic::GenISA_TraceRayAsync;

    Value* bitField = getTraceRayPayload(
        bvhLevel, traceRayCtrlVal, isRayQuery, PayloadName);
    Function* traceFn = GenISAIntrinsic::getDeclaration(
        module,
        ID,
        GlobalPointer->getType());

    Value* Args[] = { GlobalPointer, bitField };

    auto* TraceRay = cast<TraceRayIntrinsic>(this->CreateCall(traceFn, Args));
    return TraceRay;
}

void RTBuilder::createReadSyncTraceRay(Value* val)
{
    Function* readFunc = GenISAIntrinsic::getDeclaration(
        this->GetInsertBlock()->getModule(),
        GenISAIntrinsic::GenISA_ReadTraceRaySync);

    this->CreateCall(readFunc, val);
}

TraceRaySyncIntrinsic* RTBuilder::createSyncTraceRay(
    Value* bvhLevel,
    Value* traceRayCtrl,
    const Twine& PayloadName)
{
    return cast<TraceRaySyncIntrinsic>(createTraceRay(bvhLevel, this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true, PayloadName));
}

TraceRaySyncIntrinsic* RTBuilder::createSyncTraceRay(
    uint32_t bvhLevel,
    Value* traceRayCtrl,
    const Twine& PayloadName)
{
    return cast<TraceRaySyncIntrinsic>(createTraceRay(this->getInt32(bvhLevel), this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true, PayloadName));
}


static BasicBlock* getUnsetPhiBlock(PHINode* PN)
{
    SmallPtrSet<BasicBlock*, 4> BBs;

    for (auto* BB : PN->blocks())
        BBs.insert(BB);

    for (auto* BB : predecessors(PN->getParent()))
    {
        if (BBs.count(BB) == 0)
            return BB;
    }

    return nullptr;
}

//If MemHit is invalid (below if case), workload might still try to access data with InstanceLeafPtr which is not set
//and is a dangling pointer. Even though this workload behavior is against API spec, but we should not "crash".
//To avoid this crash, we check if the data request is invalid here, if it's invalid, we return an invalid data (uint32: 0xFFFFFFFF, float: INF)
// if (forCommitted && committedHit::valid == 0) ||
//     (!forCommitted && potentialHit::leafType == NODE_TYPE_INVALID))
//    value = UINT_MAX/0
// else
//    get value as usual
//----------------------------------
//% valid36 = ...
//br i1 % valid36, label% validLeafBB, label% invalidLeafBB
//
//validLeafBB : ; preds = % ProceedEndBlock
//; later, insert logic to get valid value with InstanceLeafPtr
//br label% validEndBlock
//
//invalidLeafBB : ; preds = % ProceedEndBlock
//br label % validEndBlock
//
//validEndBlock : ; preds = % invalidLeafBB, % validLeafBB
//% 97 = phi i32[-1, % invalidLeafBB] //later, valid value should be inserted here
//----------------------------------
//return value:
//  BasicBlock* is validLeafBB to let caller insert logic to this BB
//  PHINode*    is %97 above to let caller add another incoming node for phi
std::pair<BasicBlock*, PHINode*> RTBuilder::validateInstanceLeafPtr(
    RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, bool forCommitted)
{
    Value* valid = nullptr;
    if (forCommitted)
    {
        valid = getHitValid(perLaneStackPtr, forCommitted);
    }
    else
    {
        valid = this->CreateICmpNE(
            getLeafType(perLaneStackPtr, forCommitted),
            this->getInt32(NODE_TYPE_INVALID),
            VALUE_NAME("validLeafType"));
    }

    auto& C = I->getContext();
    auto* CSBlock = I->getParent();
    auto* endBlock = CSBlock->splitBasicBlock(I, VALUE_NAME("validEndBlock"));
    Function* F = this->GetInsertBlock()->getParent();

    BasicBlock* validLeafBB = BasicBlock::Create(C, VALUE_NAME("validLeafBB"), F, endBlock);
    BasicBlock* invalidLeafBB = BasicBlock::Create(C, VALUE_NAME("invalidLeafBB"), F, endBlock);
    CSBlock->getTerminator()->eraseFromParent();
    this->SetInsertPoint(CSBlock);
    this->CreateCondBr(valid, validLeafBB, invalidLeafBB);

    this->SetInsertPoint(validLeafBB);
    this->CreateBr(endBlock);

    this->SetInsertPoint(invalidLeafBB);
    Value* invalidVal = nullptr;
    if (I->getType()->isIntegerTy(32))
        invalidVal = this->getInt32(IGC_GET_FLAG_VALUE(RTInValidDefaultIndex));
    else if (I->getType()->isFloatTy())
        invalidVal = ConstantFP::getInfinity(this->getFloatTy());
    else
    {
        IGC_ASSERT_MESSAGE(0, "Unsupported datatype.");
    }

    this->CreateBr(endBlock);

    //END block
    this->SetInsertPoint(I);
    PHINode* phi = this->CreatePHI(I->getType(), 2);
    phi->addIncoming(invalidVal, invalidLeafBB);

    return std::make_pair(validLeafBB, phi);
}

// Return an allocation to `size` number of RayQueryObject.
//  Note that we separate RTStack/SMStack and RTCtrl to help reduce the ShadowMemory size
//  by hoping mem2reg kicks in to lower RTCtrl to GRF
//For example:
//
// builder.createAllocaRayQueryObjects(5)
// ==>
//  if(ShrinkShadowMemoryIfNoSpillfill)
//      %"&ShadowMemory.RayQueryObjects".first = alloca [5 x %"struct.RTStackFormat::SMStack2"]
//  else
//      % "&ShadowMemory.RayQueryObjects".first = alloca[5 x % "struct.RTStackFormat::RTStack2"]
//  % "&ShadowMemory.RayQueryObjects".second = alloca[5 x int8]
//
std::pair<Value*, Value*> RTBuilder::createAllocaRayQueryObjects(unsigned int size, bool bShrinkSMStack, const llvm::Twine& Name)
{
    //FIXME: Temp solution: to shrink ShadowMemory, if ShrinkShadowMemoryIfNoSpillfill is true (we know no spillfill), then,
    //we alloca SMStack instead of RTStack to reduce the size. Also, here, we simply cast SMStack pointer to RTStack which is risky
    //and it's safe now if we only shrink the last variable of RTStack (MemTravStack).
    //But don't shrink data in the middle of RTStack which will lead to holes.
    auto* RTStackTy = bShrinkSMStack ? getSMStack2Ty() : getRTStack2Ty();

    Value* rtStacks = this->CreateAlloca(
        ArrayType::get(RTStackTy, size),
        nullptr,
        Name);

    //FIXME: temp solution
    //one example is below, right now, we don't respect below DW alignment.
    //as a result, if we alloca <1xi8> here which alloca memory below %0, later, %0's store will have alignment issue
    //to WA this, let's temporily use DW, but sure, this won't solve all other issues.
    //  %0 = alloca <3 x float>
    //  store <3 x float> zeroinitializer, <3 x float>* %0
    Value* rtCtrls = this->CreateAlloca(
        ArrayType::get(this->getInt32Ty(), size),
        nullptr,
        Name);

    return std::make_pair(rtStacks, rtCtrls);
}

void RTBuilder::setDoneBit(RTBuilder::StackPointerVal* StackPointer, bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        _setDoneBit_##X(                  \
            StackPointer,                 \
            VAdapt{ *this, Committed });  \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}


uint32_t RTBuilder::getSyncStackSize()
{
    return Ctx.getModuleMetaData()->rtInfo.RayQueryAllocSizeInBytes;
}

uint32_t RTBuilder::getNumSyncStackSlots()
{
    return MaxDualSubSlicesSupported * Ctx.platform.getRTStackDSSMultiplier();
}


Value* RTBuilder::alignVal(Value* V, uint64_t Align)
{
    // Aligned = (V + Mask) & ~Mask;
    IGC_ASSERT_MESSAGE(iSTD::IsPowerOfTwo(Align), "Must be power-of-two!");
    IGC_ASSERT_MESSAGE(V->getType()->isIntegerTy(), "not an int?");

    uint32_t NumBits = V->getType()->getIntegerBitWidth();
    uint64_t Mask = Align - 1;
    Value* Aligned = this->CreateAnd(
        this->CreateAdd(V, this->getIntN(NumBits, Mask)),
        this->getIntN(NumBits, ~Mask),
        VALUE_NAME(V->getName() + Twine("-align-") + Twine(Align)));

    return Aligned;
}

Value* RTBuilder::getRayInfo(StackPointerVal* perLaneStackPtr, uint32_t Idx, uint32_t BvhLevel)
{
    IGC_ASSERT_MESSAGE(Idx < 8, "out-of-bounds!");
    switch (getMemoryStyle())
    {
#define STYLE(X)                       \
    case RTMemoryStyle::X:             \
        return _getRayInfo_##X(        \
            perLaneStackPtr,           \
            VAdapt{ *this, Idx },      \
            VAdapt{ *this, BvhLevel }, \
            VALUE_NAME("RayInfo." + Twine(Idx)));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getRayTMin(RTBuilder::StackPointerVal* perLaneStackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                     \
    case RTMemoryStyle::X:           \
        return _getRayTMin_##X(      \
            perLaneStackPtr,         \
            VALUE_NAME("rayTMin"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

// returns an i16 value containing the current rayflags. Sync only.
Value* RTBuilder::getRayFlags(RTBuilder::SyncStackPointerVal* perLaneStackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        return _getRayFlagsSync_##X(      \
            perLaneStackPtr,              \
            VALUE_NAME("rayflags"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

void RTBuilder::setRayFlags(RTBuilder::SyncStackPointerVal* perLaneStackPtr, Value* V)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        _setRayFlagsSync_##X(             \
            perLaneStackPtr,              \
            V);                           \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}


Value* RTBuilder::getWorldRayOrig(RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                     \
    case RTMemoryStyle::X:           \
        return _getWorldRayOrig_##X( \
            perLaneStackPtr,         \
            VAdapt{ *this, dim },    \
            VALUE_NAME("WorldRayOrig[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getMemRayOrig(StackPointerVal* perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine& Name)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                       \
    case RTMemoryStyle::X:             \
        return _getMemRayOrig_##X(     \
            perLaneStackPtr,           \
            VAdapt{ *this, dim },      \
            VAdapt{ *this, BvhLevel }, \
            Name);
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getMemRayDir(StackPointerVal* perLaneStackPtr, uint32_t dim, uint32_t BvhLevel, const Twine& Name)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                       \
    case RTMemoryStyle::X:             \
        return _getMemRayDir_##X(      \
            perLaneStackPtr,           \
            VAdapt{ *this, dim },      \
            VAdapt{ *this, BvhLevel }, \
            Name);
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getWorldRayDir(RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                       \
    case RTMemoryStyle::X:             \
        return _getWorldRayDir_##X(    \
            perLaneStackPtr,           \
            VAdapt{ *this, dim },      \
            VALUE_NAME("WorldRayDir[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getObjRayOrig(
    RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim, IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I, bool checkInstanceLeafPtr)
{
    Value* info = nullptr;
    const bool transformWorldToObject =
        ShaderTy == CallableShaderTypeMD::ClosestHit;
    if (transformWorldToObject)
    {
        info = this->TransformWorldToObject(perLaneStackPtr, dim, true, ShaderTy, I, checkInstanceLeafPtr);
    }
    else
    {
        info = getMemRayOrig(
            perLaneStackPtr,
            dim,
            BOTTOM_LEVEL_BVH,
            VALUE_NAME("ObjRayOrig[" + Twine(dim) + "]"));
    }
    return info;
}

Value* RTBuilder::getObjRayDir(
    RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim, IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I, bool checkInstanceLeafPtr)
{
    Value* info = nullptr;
    const bool transformWorldToObject =
        ShaderTy == CallableShaderTypeMD::ClosestHit;
    if (transformWorldToObject)
    {
        info = this->TransformWorldToObject(perLaneStackPtr, dim, false, ShaderTy, I, checkInstanceLeafPtr);
    }
    else
    {
        info = getMemRayDir(
            perLaneStackPtr,
            dim,
            BOTTOM_LEVEL_BVH,
            VALUE_NAME("ObjRayDir[" + Twine(dim) + "]"));
    }
    return info;
}

Value* RTBuilder::getRayTCurrent(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                     \
    case RTMemoryStyle::X:                           \
        return _getRayTCurrent_##X(                  \
            perLaneStackPtr,                         \
            VAdapt{ *this, ShaderTy },               \
            VALUE_NAME("rayTCurrent"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getInstanceIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I, bool checkInstanceLeafPtr)
{
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr))
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        Value* validVal = getInstanceIndex(perLaneStackPtr, ShaderTy);
        PN->addIncoming(validVal, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return getInstanceIndex(perLaneStackPtr, ShaderTy);
    }
}

Value* RTBuilder::getInstanceIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                     \
    case RTMemoryStyle::X:                           \
        return _getInstanceIndex_##X(                \
            perLaneStackPtr,                         \
            VAdapt{ *this, ShaderTy },               \
            VALUE_NAME("InstanceIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getInstanceID(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I, bool checkInstanceLeafPtr)
{
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr))
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        Value* validVal = getInstanceID(perLaneStackPtr, ShaderTy);
        PN->addIncoming(validVal, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return getInstanceID(perLaneStackPtr, ShaderTy);
    }
}

Value* RTBuilder::getInstanceID(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                     \
    case RTMemoryStyle::X:                           \
        return _getInstanceID_##X(                   \
            perLaneStackPtr,                         \
            VAdapt{ *this, ShaderTy },               \
            VALUE_NAME("InstanceID"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getPrimitiveIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    Instruction* I,
    Value* leafType,
    IGC::CallableShaderTypeMD ShaderTy,
    bool checkInstanceLeafPtr)
{
    uint32_t mask = (ShaderTy == CallableShaderTypeMD::ClosestHit ? 1 : 2);
    bool bMask = (IGC_GET_FLAG_VALUE(ForceRTCheckInstanceLeafPtrMask) & mask);
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr) && bMask)
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        auto* validPrimIndex = getPrimitiveIndex(perLaneStackPtr, leafType, ShaderTy == ClosestHit);
        PN->addIncoming(validPrimIndex, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return getPrimitiveIndex(perLaneStackPtr, leafType, ShaderTy == ClosestHit);
    }
}

PHINode* RTBuilder::getPrimitiveIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, Value* leafType, bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                        \
    case RTMemoryStyle::X:              \
        return _getPrimitiveIndex_##X(  \
            perLaneStackPtr,            \
            leafType,                   \
            VAdapt{ *this, Committed }, \
            VALUE_NAME("primitiveIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getGeometryIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    Instruction* I,
    Value* leafType,
    IGC::CallableShaderTypeMD ShaderTy,
    bool checkInstanceLeafPtr)
{
    uint32_t mask = (ShaderTy == CallableShaderTypeMD::ClosestHit ? 1 : 2);
    bool bMask = (IGC_GET_FLAG_VALUE(ForceRTCheckInstanceLeafPtrMask) & mask);
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr) && bMask)
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        Value* validGeomIndex = getGeometryIndex(perLaneStackPtr, &*BB->rbegin(), leafType, ShaderTy);
        PN->addIncoming(validGeomIndex, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return getGeometryIndex(perLaneStackPtr, I, leafType, ShaderTy);
    }
}

Value* RTBuilder::getGeometryIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    Instruction* I,
    Value* leafType,
    IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                     \
    case RTMemoryStyle::X:                           \
        return _getGeometryIndex_##X(                \
            perLaneStackPtr,                         \
            leafType,                                \
            VAdapt{ *this, ShaderTy == ClosestHit }, \
            VALUE_NAME("geometryIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getInstanceContributionToHitGroupIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                            \
    case RTMemoryStyle::X:                                  \
        return _getInstanceContributionToHitGroupIndex_##X( \
            perLaneStackPtr,                                \
            VAdapt{ *this, ShaderTy },                      \
            VALUE_NAME("InstanceContributionToHitGroupIndex"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getRayMask(
    RTBuilder::StackPointerVal* perLaneStackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                            \
    case RTMemoryStyle::X:                                  \
        return _getRayMask_##X(                             \
            perLaneStackPtr,                                \
            VALUE_NAME("RayMask"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getObjToWorld(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    uint32_t dim,
    IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I,
    bool checkInstanceLeafPtr)
{
    return getObjWorldAndWorldObj(perLaneStackPtr, IGC::OBJECT_TO_WORLD, dim, ShaderTy, I, checkInstanceLeafPtr);
}

Value* RTBuilder::getWorldToObj(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    uint32_t dim,
    IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I,
    bool checkInstanceLeafPtr)
{
    return getObjWorldAndWorldObj(perLaneStackPtr, IGC::WORLD_TO_OBJECT, dim, ShaderTy, I, checkInstanceLeafPtr);
}

Value* RTBuilder::getObjWorldAndWorldObj(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    uint32_t infoKind,
    uint32_t dim,
    IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I,
    bool checkInstanceLeafPtr)
{
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr))
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        Value* validVal = getObjWorldAndWorldObj(perLaneStackPtr, infoKind, dim, ShaderTy);
        PN->addIncoming(validVal, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return getObjWorldAndWorldObj(perLaneStackPtr, infoKind, dim, ShaderTy);
    }
}

Value* RTBuilder::getObjWorldAndWorldObj(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    uint32_t infoKind,
    uint32_t dim,
    IGC::CallableShaderTypeMD ShaderTy)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                               \
    case RTMemoryStyle::X:                                     \
        return _getObjWorldAndWorldObj_##X(                    \
            perLaneStackPtr,                                   \
            VAdapt{ *this, dim },                              \
            VAdapt{ *this, infoKind == IGC::OBJECT_TO_WORLD }, \
            VAdapt{ *this, ShaderTy },                         \
            VALUE_NAME("matrix[" + Twine(dim) + "]"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::TransformWorldToObject(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    unsigned int dim,
    bool isOrigin,
    IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I,
    bool checkInstanceLeafPtr)
{
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr))
    {
        auto [ValidBB, PN] =
            validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(ValidBB->getTerminator());
        Value* validVal = TransformWorldToObject(perLaneStackPtr, dim, isOrigin, ShaderTy);
        PN->addIncoming(validVal, getUnsetPhiBlock(PN));
        this->SetInsertPoint(I);
        return PN;
    }
    else
    {
        return TransformWorldToObject(perLaneStackPtr, dim, isOrigin, ShaderTy);
    }
}

Value* RTBuilder::TransformWorldToObject(
    RTBuilder::StackPointerVal* perLaneStackPtr,
    unsigned int dim,
    bool isOrigin,
    IGC::CallableShaderTypeMD ShaderTy)
{
    IGC_ASSERT_MESSAGE((dim < 3), "dim out of bounds!");

    switch (getMemoryStyle())
    {
#define STYLE(X)                            \
    case RTMemoryStyle::X:                  \
        return _TransformWorldToObject_##X( \
            perLaneStackPtr,                \
            VAdapt{ *this, dim },           \
            VAdapt{ *this, isOrigin },      \
            VAdapt{ *this, ShaderTy });
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}


Value* RTBuilder::getTraceRayPayload(
    Value* bvhLevel,
    Value* traceRayCtrl,
    bool isRayQuery,
    const Twine& PayloadName)
{
    Value* stackId = nullptr;

    // Trace message
    //Generate Payload to trace Ray which is 32-bit that includes:
    //uint8_t      bvhLevel     // the level tells the hardware which ray to process
    //uint8_t      traceRayCtrl // the command the hardware should perform
    //uint16_t     stackID      // the ID of the stack of this thread
    // Get stackID
    if (isRayQuery == true)
    {
        //For RayQuery/TraceRayInline - set stack ID to zero since hardware will not use this field
        stackId = this->getInt32(0);
    }

    constexpr uint16_t StackIDOffset =
        offsetof(TraceRayMessage::Payload, stackID) * 8;
    Value* bitField = this->CreateOr(
        traceRayCtrl,
        this->CreateShl(stackId, StackIDOffset));
    bitField = this->CreateOr(bitField, bvhLevel, PayloadName);
    return bitField;
}

GenIntrinsicInst* RTBuilder::getSr0_0()
{
    Module* module = this->GetInsertBlock()->getModule();

    auto* sr0_0 = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            module, GenISAIntrinsic::GenISA_getSR0_0),
        None,
        VALUE_NAME("sr0.0"));
    return cast<GenIntrinsicInst>(sr0_0);
}

Value* RTBuilder::emitStateRegID(uint32_t BitStart, uint32_t BitEnd)
{
    Value* sr0_0 = getSr0_0();

    uint32_t and_imm = BITMASK_RANGE(BitStart, BitEnd);
    uint32_t shr_imm = BitStart;

    Value* andInst = this->CreateAnd(
        sr0_0,
        this->getInt32(and_imm),
        VALUE_NAME("andInst"));

    Value* shrInst = this->CreateLShr(
        andInst,
        this->getInt32(shr_imm),
        VALUE_NAME("shrInst"));
    return shrInst;
}

std::pair<uint32_t, uint32_t> RTBuilder::getSliceIDBitsInSR0() const {
    if (Ctx.platform.GetPlatformFamily() == IGFX_GEN8_CORE ||
        Ctx.platform.GetPlatformFamily() == IGFX_GEN9_CORE)
    {
        return {14, 15};
    }
    else if (Ctx.platform.GetPlatformFamily() == IGFX_GEN12_CORE   ||
        Ctx.platform.GetPlatformFamily() == IGFX_XE_HP_CORE   ||
        Ctx.platform.GetPlatformFamily() == IGFX_XE_HPG_CORE)
    {
        return {11, 13};
    }
    else if (Ctx.platform.GetPlatformFamily() == IGFX_XE_HPC_CORE)
    {
        return {12, 14};
    }
    else
    {
        return {12, 14};
    }
}


std::pair<uint32_t, uint32_t> RTBuilder::getDualSubsliceIDBitsInSR0() const {
    if (Ctx.platform.GetPlatformFamily() == IGFX_GEN11_CORE   ||
        Ctx.platform.GetPlatformFamily() == IGFX_GEN11LP_CORE ||
        Ctx.platform.GetPlatformFamily() == IGFX_GEN12LP_CORE ||
        Ctx.platform.GetPlatformFamily() == IGFX_XE_HPC_CORE)
    {
        return {9, 11};
    }
    else if (Ctx.platform.GetPlatformFamily() == IGFX_GEN12_CORE   ||
        Ctx.platform.GetPlatformFamily() == IGFX_XE_HP_CORE   ||
        Ctx.platform.GetPlatformFamily() == IGFX_XE_HPG_CORE)
    {
        return {9, 10};
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "No support for Dual Subslice in current platform");
        return {0,0};
    }
}


// globalDSSID is the combined value of sliceID and dssID on slice.
Value* RTBuilder::getGlobalDSSID()
{
    {
        auto dssIDBits = getDualSubsliceIDBitsInSR0();
        auto sliceIDBits = getSliceIDBitsInSR0();

        if (dssIDBits.first < sliceIDBits.first && sliceIDBits.first == dssIDBits.second + 1)
        {
            return emitStateRegID(dssIDBits.first, sliceIDBits.second);
        }
        else
        {
            Value* dssID = emitStateRegID(dssIDBits.first, dssIDBits.second);
            Value* sliceID = emitStateRegID(sliceIDBits.first, sliceIDBits.second);
            unsigned shiftAmount = dssIDBits.second - dssIDBits.first + 1;
            Value* globalDSSID = CreateShl(sliceID, shiftAmount);
            return CreateOr(globalDSSID, dssID);
        }
    }
}


Value* RTBuilder::getRootNodePtr(Value* BVHPtr)
{
    if (IGC_IS_FLAG_ENABLED(ForceNullBVH))
    {
        // Force all BVHs to be null. Infinitely fast ray traversal.
        return this->getInt64(0);
    }

    Value* BVHI = this->CreatePointerCast(
        BVHPtr,
        this->getInt64Ty(),
        VALUE_NAME("&BVH"));

    return _getBVHPtr(
        BVHI,
        getInt64(Ctx.bvhInfo.hasFixedOffset ? Ctx.bvhInfo.offset : 0),
        getInt1(Ctx.bvhInfo.hasFixedOffset),
        VALUE_NAME("rootNodePtr"));
}

void RTBuilder::setHitT(
    StackPointerVal* StackPointer,
    Value* V,
    bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        _setHitT_##X(                     \
            StackPointer,                 \
            V,                            \
            VAdapt{ *this, Committed });  \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}

Value* RTBuilder::getHitT(
    StackPointerVal* StackPointer,
    bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        return _getHitT_##X(              \
            StackPointer,                 \
            VAdapt{ *this, Committed },   \
            VALUE_NAME(Committed ? "committedHit.t" : "potentialHit.t"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::isDoneBitNotSet(StackPointerVal* StackPointer, bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                          \
    case RTMemoryStyle::X:                \
        return _isDoneBitNotSet_##X(      \
            StackPointer,                 \
            VAdapt{ *this, Committed },   \
            VALUE_NAME("!done"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getBvhLevel(StackPointerVal* StackPointer, bool Committed)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                      \
    case RTMemoryStyle::X:            \
        return _getBvhLevel_##X(      \
            StackPointer,             \
            VAdapt{ *this, Committed });
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getHitValid(RTBuilder::StackPointerVal* StackPointer, bool CommittedHit)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                            \
    case RTMemoryStyle::X:                  \
        return _isValid_##X(                \
            StackPointer,                   \
            VAdapt{ *this, CommittedHit },  \
            VALUE_NAME("valid"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

void RTBuilder::setHitValid(StackPointerVal* StackPointer, bool CommittedHit)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                            \
    case RTMemoryStyle::X:                  \
        _setHitValid_##X(                   \
            StackPointer,                   \
            VAdapt{ *this, CommittedHit }); \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}

Value* RTBuilder::getSyncTraceRayControl(Value* ptrCtrl)
{
    return this->CreateLoad(ptrCtrl, VALUE_NAME("rayQueryObject.traceRayControl"));
}

void RTBuilder::setSyncTraceRayControl(Value* ptrCtrl, unsigned ctrl)
{
    Type* eleType = IGCLLVM::getNonOpaquePtrEltTy(ptrCtrl->getType());
    this->CreateStore(llvm::ConstantInt::get(eleType, ctrl), ptrCtrl);
}

Value* RTBuilder::getHitBaryCentric(
    RTBuilder::StackPointerVal* StackPointer, uint32_t idx, bool CommittedHit)
{
    IGC_ASSERT_MESSAGE(idx == 0 || idx == 1, "Only U V are supported.");
    switch (getMemoryStyle())
    {
#define STYLE(X)                            \
    case RTMemoryStyle::X:                  \
        return _getHitBaryCentric_##X(      \
            StackPointer,                   \
            VAdapt{ *this, idx },           \
            VAdapt{ *this, CommittedHit },  \
            VALUE_NAME(idx ? "MemHit.v" : "MemHit.u"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}


Value* RTBuilder::getLeafType(
    StackPointerVal* StackPointer, bool CommittedHit)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                            \
    case RTMemoryStyle::X:                  \
        return _createLeafType_##X(         \
            StackPointer,                   \
            VAdapt{ *this, CommittedHit },  \
            VALUE_NAME("leafType"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}


CallInst* RTBuilder::CreateLSCFence(
    LSC_SFID SFID, LSC_SCOPE Scope, LSC_FENCE_OP FenceOp)
{
    Function* pFunc = GenISAIntrinsic::getDeclaration(
        this->GetInsertBlock()->getModule(),
        GenISAIntrinsic::GenISA_LSCFence);

    Value* VSFID  = this->getInt32(SFID);
    Value* VScope = this->getInt32(Scope);
    Value* VOp    = this->getInt32(FenceOp);

    Value* Args[] =
    {
        VSFID, VScope, VOp
    };

    return this->CreateCall(pFunc, Args);
}

Value* RTBuilder::canonizePointer(Value* Ptr)
{
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
    switch (getMemoryStyle())
    {
    case RTMemoryStyle::Xe:
        VA_MSB = 48 - 1;
        break;
    }
    constexpr uint32_t MSB = 63;
    uint32_t ShiftAmt = MSB - VA_MSB;
    auto *P2I = this->CreateBitOrPointerCast(Ptr, this->getInt64Ty());
    auto* Canonized = this->CreateShl(P2I, ShiftAmt);
    Canonized = this->CreateAShr(Canonized, ShiftAmt);
    Canonized = this->CreateBitOrPointerCast(Canonized, Ptr->getType(),
        VALUE_NAME(Ptr->getName() + Twine("-canonized")));

    return Canonized;
}


void RTBuilder::setReturnAlignment(CallInst* CI, uint32_t AlignVal)
{
    auto Attrs = CI->getAttributes();
    IGCLLVM::AttrBuilder AB { CI->getContext(), Attrs.getAttributes(AttributeList::ReturnIndex)};
    AB.addAlignmentAttr(AlignVal);
    auto AL =
        IGCLLVM::addAttributesAtIndex(Attrs, CI->getContext(), AttributeList::ReturnIndex, AB);
    CI->setAttributes(AL);
}

void RTBuilder::setDereferenceable(CallInst* CI, uint32_t Size)
{
    auto Attrs = CI->getAttributes();
    IGCLLVM::AttrBuilder AB{ CI->getContext(), Attrs.getAttributes(AttributeList::ReturnIndex) };
    AB.addDereferenceableAttr(Size);
    auto AL =
        IGCLLVM::addAttributesAtIndex(Attrs, CI->getContext(), AttributeList::ReturnIndex, AB);
    CI->setAttributes(AL);
}

Value* RTBuilder::getGlobalBufferPtr()
{
    // If explicit GlobalBuffer pointer is set, use it instead
    // of getting it via intrinsic.
    // This might be the case on e.g. OpenCL path.
    if (this->GlobalBufferPtr) {
        return this->GlobalBufferPtr;
    }

    auto* M = this->GetInsertBlock()->getModule();

    auto* PtrTy = this->getRayDispatchGlobalDataPtrTy(*M);

    Function* Func = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_GlobalBufferPointer,
        PtrTy);

    CallInst *CI = this->CreateCall(Func, None, VALUE_NAME("globalPtr"));

    if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        this->setReturnAlignment(CI, RTGlobalsAlign);

    return CI;
}


Value* RTBuilder::getSyncStackID()
{
    auto& PlatformInfo = Ctx.platform.getPlatformInfo();

    if (PlatformInfo.eProductFamily == IGFX_DG2        ||
        PlatformInfo.eProductFamily == IGFX_METEORLAKE)
    {
        return _getSyncStackID_Xe(VALUE_NAME("SyncStackID"));
    }
    else if (PlatformInfo.eRenderCoreFamily == IGFX_XE_HPC_CORE)
    {
        return _getSyncStackID_Xe_HPC(VALUE_NAME("SyncStackID"));
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Invalid Product Family for SyncStackID");
    }

    return {};
}

bool RTBuilder::checkAlign(
    Module& M,
    StructType* StructTy,
    uint32_t Align)
{
    auto& DL = M.getDataLayout();
    auto Layout = DL.getStructLayout(StructTy);

    uint64_t Size = Layout->getSizeInBytes();
    uint64_t Diff = Size % Align;
    return (Diff == 0);
}


uint32_t RTBuilder::getSWStackStatefulAddrSpace(const ModuleMetaData &MMD)
{
    auto& rtInfo = MMD.rtInfo;
    IGC_ASSERT_MESSAGE(rtInfo.SWStackAddrspace != UINT_MAX, "not initialized?");
    return rtInfo.SWStackAddrspace;
}

uint32_t RTBuilder::getRTSyncStackStatefulAddrSpace(const ModuleMetaData& MMD)
{
    auto& rtInfo = MMD.rtInfo;
    IGC_ASSERT_MESSAGE(rtInfo.RTSyncStackAddrspace != UINT_MAX, "not initialized?");
    return rtInfo.RTSyncStackAddrspace;
}


static const char* RaytracingTypesMDName = "igc.magic.raytracing.types";

// If you need to add another type to the metadata, add a slot for it here
// and add create a getter function below to retrieve it.
enum class RaytracingType
{
    RTStack2Stateless     = 0,
    RTStack2Stateful      = 1,
    RayDispatchGlobalData = 2,
    SWHotZone             = 3,
    NUM_TYPES
};

// Initialize the metadata with the number of types marked as undef.  These
// will later be updated to null values with the actual types.
NamedMDNode* initTypeMD(Module& M, uint32_t NumEntries)
{
    auto *TypesMD = M.getOrInsertNamedMetadata(RaytracingTypesMDName);
    auto* Val = UndefValue::get(Type::getInt8PtrTy(M.getContext()));
    auto* Node = MDNode::get(M.getContext(), ConstantAsMetadata::get(Val));

    for (uint32_t i = 0; i < NumEntries; i++)
        TypesMD->addOperand(Node);

    return TypesMD;
}

static Type* setRTTypeMD(
    Module &M,
    RaytracingType Idx,
    NamedMDNode *TypesMD,
    Type* Ty,
    uint64_t ExpectedSize,
    uint32_t AddrSpace)
{
    IGC_ASSERT_MESSAGE(ExpectedSize == M.getDataLayout().getTypeAllocSize(Ty), "mismatch?");
    // Replace the undef value as set from initTypeMD() with a null pointer
    // to indicate that we have the type in place.
    auto* Val = ConstantPointerNull::get(Ty->getPointerTo(AddrSpace));
    auto* Node = MDNode::get(M.getContext(), ConstantAsMetadata::get(Val));
    TypesMD->setOperand((uint32_t)Idx, Node);
    return Val->getType();
}

static Type* lazyGetRTType(
    Module& M,
    RaytracingType Idx,
    std::function<Type*(NamedMDNode*, RaytracingType)> ForceFn)
{
    auto* TypesMD = M.getNamedMetadata(RaytracingTypesMDName);
    if (!TypesMD)
        TypesMD = initTypeMD(M, (uint32_t)RaytracingType::NUM_TYPES);

    auto* TypesNode = TypesMD->getOperand((uint32_t)Idx);

    auto* MD = TypesNode->getOperand(0).get();
    auto* C = cast<ConstantAsMetadata>(MD);
    if (isa<UndefValue>(C->getValue()))
    {
        return ForceFn(TypesMD, Idx);
    }

    return C->getValue()->getType();
}

Type* RTBuilder::getSMStack2Ty() const
{
    switch (getMemoryStyle())
    {
#define STYLE(X) case RTMemoryStyle::X: return _gettype_SMStack2_##X(*Ctx.getModule());
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Type* RTBuilder::getRTStack2Ty() const
{
    switch (getMemoryStyle())
    {
#define STYLE(X) case RTMemoryStyle::X: return _gettype_RTStack2_##X(*Ctx.getModule());
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
    IGC_ASSERT(0);
    return {};
}

Type* RTBuilder::getRTStack2PtrTy(
    RTBuilder::RTMemoryAccessMode Mode, bool async) const
{
    IGC_ASSERT_MESSAGE((Mode == RTBuilder::STATELESS || Mode == RTBuilder::STATEFUL), "unknown?");

    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        uint32_t AddrSpace = ADDRESS_SPACE_NUM_ADDRESSES;
        if (Mode == RTBuilder::STATELESS)
        {
            AddrSpace = ADDRESS_SPACE_GLOBAL;
        }
        else
        {
            {
                AddrSpace = getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData());
            }
        }
        IGC_ASSERT(AddrSpace != ADDRESS_SPACE_NUM_ADDRESSES);

        return setRTTypeMD(
            *Ctx.getModule(),
            Idx,
            TypesMD,
            getRTStack2Ty(),
            RTStackFormat::getRTStackHeaderSize(MAX_BVH_LEVELS),
            AddrSpace);
    };

    auto RTType = (Mode == RTBuilder::STATELESS) ?
        RaytracingType::RTStack2Stateless : RaytracingType::RTStack2Stateful;

    return lazyGetRTType(*Ctx.getModule(), RTType, addTy);
}

Type* RTBuilder::getRayDispatchGlobalDataPtrTy(Module &M)
{
    uint32_t Addrspace = (Ctx.type == ShaderType::OPENCL_SHADER) ?
        ADDRESS_SPACE_GLOBAL :
        ADDRESS_SPACE_CONSTANT;

    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_RayDispatchGlobalData(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(RayDispatchGlobalData),
            Addrspace);
    };

    return lazyGetRTType(M, RaytracingType::RayDispatchGlobalData, addTy);
}


// Find the Insert point which is placed
// after all Allocas and after all the Instructions
// from the optional vector additionalInstructionsToSkip.
Instruction* RTBuilder::getEntryFirstInsertionPt(
    Function& F,
    const std::vector<Value*>* additionalInstructionsToSkip)
{
    auto& EntryBB = F.getEntryBlock();

    // The insert point will be right after the last alloca
    // assumes a well-formed block with a terminator at the end
    auto* CurIP = &*EntryBB.begin();
    for (auto& I : EntryBB)
    {
        bool skipInstruction = false;

        if (additionalInstructionsToSkip != nullptr)
        {
            for (const Value* inst : *additionalInstructionsToSkip)
            {
                if (inst == &I)
                {
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

Type* RTBuilder::getInt64PtrTy(unsigned int AddrSpace) const
{
    return Type::getInt64PtrTy(this->Context, AddrSpace);
}

Type* RTBuilder::getInt32PtrTy(unsigned int AddrSpace) const
{
    return Type::getInt32PtrTy(this->Context, AddrSpace);
}

IGC::RTMemoryStyle RTBuilder::getMemoryStyle() const
{
    return Ctx.getModuleMetaData()->rtInfo.MemStyle;
}


SpillValueIntrinsic* RTBuilder::getSpillValue(Value* Val, uint64_t Offset)
{
    auto* M = this->GetInsertBlock()->getModule();
    Function* SpillFunction = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_SpillValue,
        Val->getType());
    // create spill instruction
    Value *Args[] = {
        Val,
        this->getInt64(Offset)
    };
    auto* SpillValue = this->CreateCall(SpillFunction, Args);

    return cast<SpillValueIntrinsic>(SpillValue);
}

FillValueIntrinsic* RTBuilder::getFillValue(
    Type *Ty, uint64_t Offset, const Twine &Name)
{
    auto* M = this->GetInsertBlock()->getModule();
    Function* FillFunction = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_FillValue,
        Ty);
    auto* FillValue = this->CreateCall(
        FillFunction,
        this->getInt64(Offset),
        Name);

    return cast<FillValueIntrinsic>(FillValue);
}

void RTBuilder::setGlobalBufferPtr(Value* GlobalBufferPtr) {
    this->GlobalBufferPtr = GlobalBufferPtr;
}

void RTBuilder::setDisableRTGlobalsKnownValues(bool Disable) {
    this->DisableRTGlobalsKnownValues = Disable;
}


CallInst* RTBuilder::ctlz(Value* V)
{
    auto* Ctlz = Intrinsic::getDeclaration(
        GetInsertBlock()->getModule(), Intrinsic::ctlz, V->getType());
    return CreateCall2(Ctlz, V, getFalse(), VALUE_NAME("lzd"));
}

void RTBuilder::createPotentialHit2CommittedHit(StackPointerVal* StackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                        \
    case RTMemoryStyle::X:                              \
        _createPotentialHit2CommittedHit_##X(StackPtr); \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}

void RTBuilder::createTraceRayInlinePrologue(
    StackPointerVal* StackPtr,
    Value* RayInfo,
    Value* RootNodePtr,
    Value* RayFlags,
    Value* InstanceInclusionMask,
    Value* TMax)
{
    switch (getMemoryStyle())
    {
    case RTMemoryStyle::Xe:
        _createTraceRayInlinePrologue_Xe(
            StackPtr,
            RayInfo,
            RootNodePtr,
            RayFlags,
            InstanceInclusionMask,
            TMax);
        break;
    }
}

void RTBuilder::emitSingleRQMemRayWrite(
    SyncStackPointerVal* HWStackPtr,
    SyncStackPointerVal* SMStackPtr,
    bool singleRQProceed)
{
    switch (getMemoryStyle())
    {
    case RTMemoryStyle::Xe:
        _emitSingleRQMemRayWrite_Xe(
            HWStackPtr,
            SMStackPtr,
            VAdapt{ *this, singleRQProceed });
        break;
    }
}

void RTBuilder::copyMemHitInProceed(
    SyncStackPointerVal* HWStackPtr,
    SyncStackPointerVal* SMStackPtr,
    bool singleRQProceed)
{
    switch (getMemoryStyle())
    {
    case RTMemoryStyle::Xe:
        _copyMemHitInProceed_Xe(
            HWStackPtr,
            SMStackPtr,
            VAdapt{ *this, singleRQProceed });
        break;
    }
}

Value* RTBuilder::syncStackToShadowMemory(
    SyncStackPointerVal* HWStackPtr,
    SyncStackPointerVal* SMStackPtr,
    Value* ProceedReturnVal,
    Value* ShadowMemRTCtrlPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                             \
    case RTMemoryStyle::X:                   \
        return _syncStackToShadowMemory_##X( \
            HWStackPtr,                      \
            SMStackPtr,                      \
            ProceedReturnVal,                \
            ShadowMemRTCtrlPtr);
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }

    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getCommittedStatus(SyncStackPointerVal* SMStackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                             \
    case RTMemoryStyle::X:                   \
        return _getCommittedStatus_##X(SMStackPtr);
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }

    IGC_ASSERT(0);
    return {};
}

Value* RTBuilder::getCandidateType(SyncStackPointerVal* SMStackPtr)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                             \
    case RTMemoryStyle::X:                   \
        return _getCandidateType_##X(SMStackPtr, VALUE_NAME("CandidateType"));
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }

    IGC_ASSERT(0);
    return {};
}

void RTBuilder::commitProceduralPrimitiveHit(
    SyncStackPointerVal* SMStackPtr, Value* THit)
{
    switch (getMemoryStyle())
    {
#define STYLE(X)                                             \
    case RTMemoryStyle::X:                                   \
        _commitProceduralPrimitiveHit_##X(SMStackPtr, THit); \
        break;
#include "RayTracingMemoryStyle.h"
#undef STYLE
    }
}
