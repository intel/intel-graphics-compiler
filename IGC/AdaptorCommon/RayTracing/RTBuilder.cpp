/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

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
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_rtMemBasePtr(BasePtr, VALUE_NAME("&rtMemBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("rtMemBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpRtMemBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pRtMemBasePtr(BasePtr, VALUE_NAME("&pRtMemBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pRtMemBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getCallStackHandler(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_callStackHandlerPtr(BasePtr, VALUE_NAME("&callStackHandlerPtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("callStackHandlerPtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getStackSizePerRay(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_stackSizePerRay(BasePtr, VALUE_NAME("&stackSizePerRay"));
    LoadInst* stackSizePerRay =
        this->CreateLoad(Ptr, VALUE_NAME("stackSizePerRay"));
    setInvariantLoad(stackSizePerRay);

    // Multiply by 64 to return the required stack size in bytes.
    Value* StackSizePerRayInBytes = this->CreateShl(
        this->CreateTrunc(stackSizePerRay, this->getInt16Ty()),
        this->getInt16(6),
        VALUE_NAME("StackSizePerRayInBytes"));

    return StackSizePerRayInBytes;
}

Value* RTBuilder::getpStackSizePerRay(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pStackSizePerRay(BasePtr, VALUE_NAME("&pStackSizePerRay"));
    LoadInst* stackSizePerRay =
        this->CreateLoad(Ptr, VALUE_NAME("pStackSizePerRay"));
    setInvariantLoad(stackSizePerRay);

    // Multiply by 64 to return the required stack size in bytes.
    Value* StackSizePerRayInBytes = this->CreateShl(
        this->CreateTrunc(stackSizePerRay, this->getInt16Ty()),
        this->getInt16(6),
        VALUE_NAME("StackSizePerRayInBytes"));

    return StackSizePerRayInBytes;
}

Value* RTBuilder::getSWStackSizePerRay(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_swStackSizePerRay(
        BasePtr,
        VALUE_NAME("&swStackSizePerRay"));
    LoadInst* SWStackSizePerRay =
        this->CreateLoad(Ptr, VALUE_NAME("swStackSizePerRay"));
    setInvariantLoad(SWStackSizePerRay);

    return SWStackSizePerRay;
}

Value* RTBuilder::getNumDSSRTStacks(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_numDSSRTStacks(BasePtr, VALUE_NAME("&numDSSRTStacks"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("numDSSRTStacks"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpNumDSSRTStacks(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pNumDSSRTStacks(BasePtr, VALUE_NAME("&pNumDSSRTStacks"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pNumDSSRTStacks"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getMaxBVHLevels(void)
{
    if (!DisableRTGlobalsKnownValues)
        return this->getInt32(MAX_BVH_LEVELS);

    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_maxBVHLevels(BasePtr, VALUE_NAME("&maxBVHLevels"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("maxBVHLevels"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getHitGroupBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_hitGroupBasePtr(BasePtr, VALUE_NAME("&hitGroupBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("hitGroupBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpHitGroupBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pHitGroupBasePtr(BasePtr, VALUE_NAME("&pHitGroupBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pHitGroupBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getMissShaderBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_missShaderBasePtr(BasePtr, VALUE_NAME("&missShaderBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("missShaderBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpMissShaderBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pMissShaderBasePtr(BasePtr, VALUE_NAME("&pMissShaderBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pMissShaderBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getCallableShaderBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_callableShaderBasePtr(BasePtr, VALUE_NAME("&callableShaderBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("callableShaderBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpCallableShaderBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pCallableShaderBasePtr(BasePtr, VALUE_NAME("&pCallableShaderBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pCallableShaderBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getBindlessHeapBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_bindlessHeapBasePtr(BasePtr, VALUE_NAME("&bindlessHeapBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("bindlessHeapBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getBaseSSHOffset(void)
{
    if (IGC_IS_FLAG_ENABLED(EnableKnownBTIBase))
        return this->getInt32(IGC_GET_FLAG_VALUE(KnownBTIBaseValue));

    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_baseSSHOffset(BasePtr, VALUE_NAME("&baseSSHOffset"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("baseSSHOffset"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getPrintfBufferBasePtr(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_printfBufferBasePtr(BasePtr, VALUE_NAME("&printfBufferBasePtr"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("printfBufferBasePtr"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getHitGroupStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_hitGroupStride(BasePtr, VALUE_NAME("&hitGroupStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("hitGroupStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpHitGroupStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pHitGroupStride(BasePtr, VALUE_NAME("&pHitGroupStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pHitGroupStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getMissShaderStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_missShaderStride(BasePtr, VALUE_NAME("&missShaderStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("missShaderStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpMissShaderStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pMissShaderStride(BasePtr, VALUE_NAME("&pMissShaderStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pMissShaderStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getCallableShaderStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_callableShaderStride(BasePtr, VALUE_NAME("&callableShaderStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("callableShaderStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getpCallableShaderStride(void)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_pCallableShaderStride(BasePtr, VALUE_NAME("&pCallableShaderStride"));
    LoadInst* Val = this->CreateLoad(Ptr, VALUE_NAME("pCallableShaderStride"));
    setInvariantLoad(Val);
    return Val;
}

Value* RTBuilder::getDispatchRayDimension(unsigned int dim)
{
    auto* BasePtr = this->getGlobalBufferPtr();
    auto* Ptr = this->_gepof_dispatchRaysDimensions(BasePtr, this->getInt32(dim));
    LoadInst* Val = this->CreateLoad(Ptr,
        VALUE_NAME("dispatchRaysDimensions[" + Twine(dim) + "]"));
    setInvariantLoad(Val);
    return Val;
}

//get MemHit.topOfPrimIndexDelta/frontFaceDword/hitInfoDWord
//to avoid confusion, let's use one single method to represent all 3 union fields
//caller could provide Name to explicitly specify which DW it's meant to use
//Note: This kind of design might cause a little issue that we cannot assert invalid ShaderTy for a specific field,
//  say, if frontFace is to be retrieved,, ShaderTy must be CSH/AnyHit.
Value* RTBuilder::getHitInfoDWordPtr(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, const Twine& Name)
{
    Value* Ptr = nullptr;
    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        Ptr = this->_gepof_CommittedHitTopOfPrimIndexDelta(
            StackPointer,
            VALUE_NAME("&committedHit." + Name));
    }
    else
    {
        Ptr = this->_gepof_PotentialHitTopOfPrimIndexDelta(
            StackPointer,
            VALUE_NAME("&potentialHit." + Name));
    }

    return Ptr;
}

Value* RTBuilder::getHitInfoDWord(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, const Twine& Name)
{
    return this->CreateLoad(this->getHitInfoDWordPtr(StackPointer, ShaderTy, Name), Name);
}

void RTBuilder::setHitInfoDWord(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, Value* V, const Twine& Name)
{
    this->CreateStore(V, this->getHitInfoDWordPtr(StackPointer, ShaderTy, Name));
}

Value* RTBuilder::getIsFrontFace(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* Val = this->getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("frontFaceDword"));
    Val = this->CreateAnd(
        Val,
        this->getInt32(1U << (uint32_t)MemHit::Offset::frontFace),
        VALUE_NAME("isolate_front_face_bit"));
    return this->CreateICmpNE(
        Val,
        this->getInt32(0),
        VALUE_NAME("is_front_face"));
}

Value* RTBuilder::getTriangleHitKind(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* IsFrontFace = getIsFrontFace(StackPointer, ShaderTy);
    Value* RetVal = this->CreateSelect(IsFrontFace,
        this->getInt32(HIT_KIND_TRIANGLE_FRONT_FACE),
        this->getInt32(HIT_KIND_TRIANGLE_BACK_FACE),
        VALUE_NAME("hit_kind_enum_val"));

    return RetVal;
}

RTBuilder::StackOffsetIntVal* RTBuilder::getFirstFrameOffset()
{
    Value* Offset = nullptr;
    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
        Offset = this->getInt32(0);
    else
        Offset = this->getInt16(0);

    return static_cast<StackOffsetIntVal*>(Offset);
}

// Given a pointer to the stack pointer location, this will bump
// the 'FrameAddr' by 'Amount', update the stack pointer, and return its new
// value.
RTBuilder::SWStackPtrVal* RTBuilder::bumpStackPtr(
    RTBuilder::SWStackPtrVal* FrameAddr,
    uint64_t Amount,
    RTBuilder::StackOffsetIntVal *FrameOffset,
    RTBuilder::StackOffsetPtrVal* Ptr)
{
    IGC_ASSERT(FrameAddr->getType()->getPointerElementType() == this->getInt8Ty());
    IGC_ASSERT_MESSAGE(Amount < std::numeric_limits<uint32_t>::max(), "overflow?");

    auto* NewStackOffsetVal = this->CreateAdd(
        FrameOffset,
        this->getIntN(FrameOffset->getType()->getIntegerBitWidth(), Amount),
        VALUE_NAME("bumped_stack_offset_val"));
    this->writeNewStackOffsetVal(NewStackOffsetVal, Ptr);

    auto* NewStackPtrVal = this->CreateGEP(
        FrameAddr,
        this->getInt32((uint32_t)Amount),
        VALUE_NAME("bumped_stack_pointer_val"));

    return static_cast<RTBuilder::SWStackPtrVal*>(NewStackPtrVal);
}

// We want to L1$ the stack pointer. Whenever we write the stack offset back
// to the hotzone, we ensure it will be at least 16 bytes wide.
void RTBuilder::writeNewStackOffsetVal(
    Value* Offset,
    RTBuilder::StackOffsetPtrVal* Ptr)
{
    if (IGC_IS_FLAG_ENABLED(EnableCompressedRayIndices))
    {
        this->CreateStore(Offset, Ptr);
        return;
    }

    IGC_ASSERT_MESSAGE(Offset->getType()->isIntegerTy(32), "changed?");
    static_assert(sizeof(SWHotZone_v2) == 16, "changed?");
    static_assert(offsetof(SWHotZone_v2, StackOffset) == 0, "changed?");

    uint32_t AddrSpace = Ptr->getType()->getPointerAddressSpace();
    Type* ArrTy = ArrayType::get(this->getInt32Ty(), 4);
    Value* ArrayPtr = this->CreateBitCast(Ptr, ArrTy->getPointerTo(AddrSpace));

    Value* GEPs[3];
    Value* DWs[3];
    for (uint32_t i = 0; i < 3; i++)
    {
        Value* Idx[] = { this->getInt32(0), this->getInt32(i + 1) };
        Value* CurPtr = this->CreateInBoundsGEP(ArrayPtr, Idx);
        GEPs[i] = CurPtr;
        DWs[i] = this->CreateLoad(CurPtr, VALUE_NAME("reload"));
    }

    //FIXME: store vector instead of scalar to avoid those 3 DWs being optimized away by DSE.
    //We might want to revisit this because DSE's behavior might be changed
    //and these 3DWs could be opted away by other passes as well.
    //LSCCacheOptimizationPass or a similar pass could help to resolve this issue totally?
    IGC_ASSERT(Offset->getType() == DWs[0]->getType());
    Type* vecType = IGCLLVM::FixedVectorType::get(Offset->getType(), 4);
    Value* vecPointer = this->CreateBitCast(Ptr, vecType->getPointerTo(AddrSpace));
    Value* vecVals = llvm::UndefValue::get(vecType);
    vecVals = this->CreateInsertElement(vecVals, Offset, getInt32(0));
    for (uint32_t i = 0; i < 3; i++)
    {
        vecVals = this->CreateInsertElement(vecVals, DWs[i], getInt32(i+1));
    }
    this->CreateAlignedStore(vecVals, vecPointer, IGCLLVM::Align(LSC_WRITE_GRANULARITY));
}

RTBuilder::StackOffsetPtrVal* RTBuilder::getAddrOfSWStackOffset(
    SWHotZonePtrVal* SWHotZonePtr)
{
    Value* Ptr = nullptr;
    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
    {
        Ptr = this->_gepof_StackOffset_v2(
            SWHotZonePtr,
            VALUE_NAME("&SWStackOffset"));
    }
    else
    {
        Ptr = this->_gepof_StackOffset_v1(
            SWHotZonePtr,
            VALUE_NAME("&SWStackOffset"));
    }
    return static_cast<StackOffsetPtrVal*>(Ptr);
}

RTBuilder::StackOffsetIntVal* RTBuilder::getSWStackOffset(
    RTBuilder::StackOffsetPtrVal* OffsetPtr)
{
    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
    {
        Value* Load = this->CreateLoad(OffsetPtr, VALUE_NAME("SWStackOffset"));
        return static_cast<StackOffsetIntVal*>(Load);
    }
    else
    {
        // Load this as a dword to make vectorization easier with the adjacent
        // dword.
        static_assert(offsetof(StackPtrAndBudges, StackOffset) == 0, "changed?");
        static_assert(sizeof(StackPtrAndBudges) >= 4, "not large enough!");
        auto* Ptr = this->CreateBitCast(
            OffsetPtr,
            this->getInt32PtrTy(OffsetPtr->getType()->getPointerAddressSpace()));
        Value* Load = this->CreateLoad(Ptr,
            VALUE_NAME("SWStackOffset"));
        Load = this->CreateBitCast(Load, IGCLLVM::FixedVectorType::get(this->getInt16Ty(), 2));
        Load = this->CreateExtractElement(Load, (uint64_t)0, VALUE_NAME("SWStackOffset"));
        return static_cast<StackOffsetIntVal*>(Load);
    }
}

RTBuilder::StackOffsetIntVal* RTBuilder::getSWStackOffset(
    SWHotZonePtrVal* SWHotZonePtr)
{
    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
    {
        Value* Ptr = this->_gepof_StackOffset_v2(
            SWHotZonePtr, VALUE_NAME("&SWStackOffset"));

        return this->getSWStackOffset(static_cast<StackOffsetPtrVal*>(Ptr));
    }
    else
    {
        Value* Ptr = this->_gepof_StackOffset_v1(
            SWHotZonePtr, VALUE_NAME("&SWStackOffset"));

        return this->getSWStackOffset(static_cast<StackOffsetPtrVal*>(Ptr));
    }
}

Value* RTBuilder::CreateSWHotZonePtrIntrinsic(
    Value *Addr,
    Type *PtrTy,
    bool AddDecoration)
{
    Module* M = this->GetInsertBlock()->getModule();
    Type* Tys[] = { PtrTy, Addr->getType() };
    CallInst* HotZone = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M, GenISAIntrinsic::GenISA_SWHotZonePtr, Tys),
        Addr,
        VALUE_NAME("&SWHotZone"));

    constexpr uint32_t Align = alignof(SWHotZone_v2);
    static_assert(Align == 16, "changed?");

    if (AddDecoration)
    {
        this->setReturnAlignment(HotZone, Align);
        if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        {
            this->setDereferenceable(HotZone, this->getSWHotZoneSize());
        }
    }

    return HotZone;
}

Value* RTBuilder::CreateAsyncStackPtrIntrinsic(
    Value* Addr, Type* PtrTy, bool AddDecoration)
{
    Module* M = this->GetInsertBlock()->getModule();
    Type* Tys[] = { PtrTy, Addr->getType() };
    CallInst* StackPtr = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M, GenISAIntrinsic::GenISA_AsyncStackPtr, Tys),
        Addr,
        VALUE_NAME("perLaneAsyncStackPointer"));

    if (AddDecoration)
    {
        this->setReturnAlignment(StackPtr, RTStackAlign);
        if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        {
            this->setDereferenceable(StackPtr, sizeof(RTStack2));
        }
    }

    return StackPtr;
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
            this->setDereferenceable(StackPtr, sizeof(RTStack2));
        }
    }

    return StackPtr;
}


RTBuilder::SWStackPtrVal* RTBuilder::getSWStackPointer(const Twine& Name)
{
    auto* CI = this->CreateSWStackPtrIntrinsic(nullptr, false, Name);
    return static_cast<RTBuilder::SWStackPtrVal*>(cast<Value>(CI));
}

RTBuilder::SWStackPtrVal*
RTBuilder::getSWStackPointer(
    Optional<RTBuilder::StackOffsetIntVal*> StackOffset,
    int32_t FrameSize,
    bool SubtractFrameSize,
    SWStackPtrVal*& CurStackVal,
    const Twine& Name)
{
    // Hop over the async stacks

    // HWStack AsyncStacks[DSS_COUNT * NumDSSRTStacks];

    Value* SWStacksBase = nullptr;

    if (Ctx.getModuleMetaData()->rtInfo.SWStackSurfaceStateOffset)
    {
        uint32_t AddrSpace =
            getSWStackStatefulAddrSpace(*Ctx.getModuleMetaData());

        SWStacksBase = Constant::getNullValue(this->getInt8PtrTy(AddrSpace));
    }
    else
    {
        Value* rtMemBasePtr = this->getpRtMemBasePtr();
        rtMemBasePtr = this->CreateBitOrPointerCast(
            rtMemBasePtr, this->getInt8PtrTy(ADDRESS_SPACE_GLOBAL));

        auto* AsyncStacksSize =
            this->CreateMul(
                this->getAsyncRTStackSize(),
                this->getNumAsyncStackSlots());

        SWStacksBase = this->CreateGEP(rtMemBasePtr, AsyncStacksSize);
    }

    Value* StackID = this->getGlobalAsyncStackID();
    Value* SWStackSizePerRay = this->getSWStackSizePerRay();

    Value* Offset = this->CreateMul(StackID, SWStackSizePerRay);

    auto* SWStack = this->CreateGEP(SWStacksBase, Offset);

    if (StackOffset)
        SWStack = this->CreateGEP(SWStack, *StackOffset);

    CurStackVal = static_cast<RTBuilder::SWStackPtrVal*>(SWStack);

    if (SubtractFrameSize)
    {
        SWStack = this->CreateGEP(
                SWStack,
                this->getInt32(-FrameSize),
            VALUE_NAME("ResetStackPointer"));
    }

    auto* CI = this->CreateSWStackPtrIntrinsic(SWStack, true, Name);

    if (!SubtractFrameSize)
        CurStackVal = static_cast<RTBuilder::SWStackPtrVal*>(cast<Value>(CI));

    if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        this->setDereferenceable(CI, FrameSize);

    return static_cast<RTBuilder::SWStackPtrVal*>(cast<Value>(CI));
}

CallInst* RTBuilder::CreateSWStackPtrIntrinsic(
    Value* Addr, bool AddDecoration, const Twine &Name)
{
    Module* M = this->GetInsertBlock()->getModule();

    uint32_t AddrSpace = getSWStackAddrSpace(*Ctx.getModuleMetaData());

    if (!Addr)
    {
        auto* AddrTy = this->getInt8PtrTy(AddrSpace);
        Addr = Constant::getNullValue(AddrTy);
    }

    auto* RetTy = this->getInt8PtrTy(AddrSpace);
    CallInst* SWStackPtr = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M, GenISAIntrinsic::GenISA_SWStackPtr, RetTy),
        Addr,
        Name);

    if (AddDecoration)
    {
        this->setReturnAlignment(SWStackPtr, StackFrameAlign);
    }

    return SWStackPtr;
}

CallInst* RTBuilder::createMergeCall()
{
    IGCLLVM::Module* M = (IGCLLVM::Module*)this->GetInsertBlock()->getModule();
    auto* FTy = FunctionType::get(
        this->getVoidTy(), this->getInt64Ty(), false);

    auto* MergeFunc = cast<Function>(M->getOrInsertFunction(
        MergeFuncName, FTy));

    // initialize with a default value that will be patched later in lowering.
    return this->CreateCall(MergeFunc, this->getInt64(-1));
}

// Calculates the address of a ray's SWHotZone.
RTBuilder::SWHotZonePtrVal* RTBuilder::getSWHotZonePointer(bool BuildAddress)
{
    auto Mode = Ctx.getModuleMetaData()->rtInfo.SWHotZoneSurfaceStateOffset ?
        RTBuilder::STATEFUL :
        RTBuilder::STATELESS;

    auto* PtrTy = this->getSWHotZonePtrTy(Ctx, Mode);

    if (!BuildAddress)
    {
        auto* Val = Constant::getNullValue(this->getInt32Ty());
        return static_cast<RTBuilder::SWHotZonePtrVal*>(
            this->CreateSWHotZonePtrIntrinsic(Val, PtrTy, false));
    }

    Value* stackID = this->getGlobalAsyncStackID();
    Value* BaseOffset =
        this->CreateMul(stackID, this->getInt32(getSWHotZoneSize()));

    // RTDispatchGlobals_HW.rtMemBasePointer -
    //    (sizeof(RTMemory.SyncStacks) + sizeof(RTMemory.HotZones)) +
    //    globalAsyncStackID * getSWHotZoneSize(); // 16B aligned
    // ===>
    // rearrange slightly to compute the offset with 32-bit arithmetic:
    //
    // RTDispatchGlobals_HW.rtMemBasePointer -
    //    ((sizeof(RTMemory.SyncStacks) + sizeof(RTMemory.HotZones)) -
    //       globalAsyncStackID * getSWHotZoneSize()); // 16B aligned
    //

    if (Mode == RTBuilder::STATEFUL)
    {
        return static_cast<RTBuilder::SWHotZonePtrVal*>(
            this->CreateSWHotZonePtrIntrinsic(BaseOffset, PtrTy, true));
    }
    else
    {
        Value* rtMemBasePtr = this->getpRtMemBasePtr();
        Value* SyncAndHotZonesSize =
            this->CreateAdd(
                this->getInt32(getSumSyncStackSize()),
                getSumSWHotZoneSize());
        Value* Offset = this->CreateSub(
            SyncAndHotZonesSize,
            BaseOffset);
        Value* Addr = this->CreateSub(
            rtMemBasePtr,
            this->CreateZExt(Offset, this->getInt64Ty()));

        return static_cast<RTBuilder::SWHotZonePtrVal*>(
            this->CreateSWHotZonePtrIntrinsic(Addr, PtrTy, true));
    }
}

Value* RTBuilder::GetAsyncStackOffset()
{
    // Per thread Asynchronous RTStack is calculated using the following formula:
    //       rtMemBasePtr + globalAsyncStackID * stackSizePerRay

    Value* stackSize = this->getAsyncRTStackSize();
    if (!isa<Constant>(stackSize))
        stackSize = this->getpStackSizePerRay();

    Value* globalStackID = this->getGlobalAsyncStackID();

    return this->CreateMul(
        globalStackID,
        this->CreateZExt(stackSize, globalStackID->getType()),
        VALUE_NAME("AsyncStackOffset"));
}

// It is the return value of this method that is passed into the 'StackPointer'
// argument of many of the functions in this class.
RTBuilder::AsyncStackPointerVal* RTBuilder::getAsyncStackPointer(bool BuildAddress)
{
    auto Mode = Ctx.getModuleMetaData()->rtInfo.RTAsyncStackSurfaceStateOffset ?
        RTBuilder::STATEFUL :
        RTBuilder::STATELESS;
    // requests for the async stack pointer in early phases of compilation
    // will just return a marker without the address computation.
    // In RayTracingFinalizePass, we will then update them to actually compute
    // the address.
    if (!BuildAddress)
    {
        auto* PtrTy = this->getRTStack2PtrTy(
            Ctx, Mode);

        auto* Val = Constant::getNullValue(PtrTy);

        return static_cast<RTBuilder::AsyncStackPointerVal*>(
            this->CreateAsyncStackPtrIntrinsic(Val, PtrTy, false));
    }

    Value* stackOffset = this->GetAsyncStackOffset();
    return this->getAsyncStackPointer(stackOffset, Mode);
}

// Returns an integer value corresponding to the address of the base
// of the per lane stack (each ray gets its own private stack).
// That is, this points to the base of the RTStack.
RTBuilder::AsyncStackPointerVal* RTBuilder::getAsyncStackPointer(
    Value* asyncStackOffset,
    RTBuilder::RTMemoryAccessMode Mode)
{
    auto* PtrTy = this->getRTStack2PtrTy(Ctx, Mode);

    if (Mode == RTBuilder::STATEFUL)
    {
        IGC_ASSERT_MESSAGE((this->Ctx.type == ShaderType::RAYTRACING_SHADER), "only raytracing shaders for now!");

        return static_cast<RTBuilder::AsyncStackPointerVal*>(
            this->CreateAsyncStackPtrIntrinsic(asyncStackOffset, PtrTy, true));
    }
    else
    {
        Value* memBasePtr = this->getpRtMemBasePtr();
        Value* stackBase = this->CreatePtrToInt(memBasePtr, this->getInt64Ty());

        Value* perLaneStackPointer = this->CreateAdd(
            stackBase,
            this->CreateZExt(asyncStackOffset, stackBase->getType()));

        return static_cast<RTBuilder::AsyncStackPointerVal*>(
            this->CreateAsyncStackPtrIntrinsic(perLaneStackPointer, PtrTy, true));
    }
}

// If we imagine the collection of the RTStacks for all rays laid out
// contiguously, this returns the ith stack in the sequence.  It is *not* an
// address, just the "global id" (not to be confused with the stack ID which is
// unique within a DSS.  This is across all DSSs).
Value* RTBuilder::getGlobalAsyncStackID()
{
    // global stack id = (dssIDGlobal * numDSSRTStacks + stackID)
    Value* IDGlobal = this->getGlobalDSSID();

    Value* stackID = this->getAsyncStackID();
    Value* numberOfDSSRTStacks = this->getpNumDSSRTStacks();

    Value* val = this->CreateMul(IDGlobal, numberOfDSSRTStacks);
    val = this->CreateAdd(
        val,
        this->CreateZExt(stackID, val->getType()));

    return val;
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

Value* RTBuilder::getRTStackSize(uint32_t Align)
{
    static_assert(sizeof(RTStack2) == 256, "Might need to update this!");
    // syncStackSize = sizeof(HitInfo)*2 + (sizeof(Ray) + sizeof(TravStack))*RTDispatchGlobals.maxBVHLevels
    Value* stackSize = this->CreateMul(
        this->getInt32(sizeof(RTStackFormat::MemRay) + sizeof(RTStackFormat::MemTravStack)),
        this->getMaxBVHLevels());

    stackSize = this->CreateAdd(
        stackSize, this->getInt32(sizeof(RTStackFormat::MemHit) * 2),
        VALUE_NAME("RTStackSize"));

    stackSize = this->alignVal(stackSize, Align);

    return stackSize;
}

Value* RTBuilder::getAsyncRTStackSize()
{
    return this->getRTStackSize(RTStackAlign);
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
    auto* PtrTy = _gettype_RTStack2(*this->Ctx.getModule());
    if (Mode == RTBuilder::STATEFUL)
    {
        uint32_t AddrSpace = getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData());
        Value* perLaneStackPointer = this->CreateIntToPtr(syncStackOffset, PointerType::get(PtrTy, AddrSpace));
        return static_cast<RTBuilder::SyncStackPointerVal*>(perLaneStackPointer);
    }
    else
    {
        Value* memBasePtr = (this->Ctx.type == ShaderType::RAYTRACING_SHADER) ?
            this->getpRtMemBasePtr() :
            this->getRtMemBasePtr();
        Value* stackBase = this->CreatePtrToInt(memBasePtr, this->getInt64Ty());
        Value* perLaneStackPointer = this->CreateSub(
            stackBase,
            this->CreateZExt(syncStackOffset, stackBase->getType()),
            VALUE_NAME("HWMem.perLaneSyncStackPointer"));
        perLaneStackPointer = this->CreateIntToPtr(perLaneStackPointer, PointerType::get(PtrTy, ADDRESS_SPACE_GLOBAL));
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
    auto* PtrTy = this->getRTStack2PtrTy(Ctx, Mode, false);

    Value* stackOffset = this->getSyncStackOffset(RTBuilder::STATELESS == Mode);
    stackOffset = this->getSyncStackPointer(stackOffset, Mode);
    return static_cast<RTBuilder::SyncStackPointerVal*>(
        this->CreateSyncStackPtrIntrinsic(stackOffset, PtrTy, true));
}

CallInst* RTBuilder::CreateBTDCall(Value* RecordPointer)
{
    Module* module = this->GetInsertBlock()->getModule();
    Value* GlobalPointer = this->getGlobalBufferPtr();
    Value* stackID = this->getAsyncStackID();

    Function* BTDCall = GenISAIntrinsic::getDeclaration(
        module,
        GenISAIntrinsic::GenISA_BindlessThreadDispatch,
        GlobalPointer->getType());

    Value* args[] = {
        GlobalPointer,
        stackID,
        RecordPointer
    };
    return this->CreateCall(BTDCall, args);
}

StackIDReleaseIntrinsic* RTBuilder::CreateStackIDRelease(
    Value* StackID, Value* Flag)
{
    Module* module = this->GetInsertBlock()->getModule();
    Value* stackID = StackID ? StackID : this->getAsyncStackID();

    Function* BTDCall = GenISAIntrinsic::getDeclaration(
        module,
        GenISAIntrinsic::GenISA_StackIDRelease);

    auto* Predicate = Flag ? Flag : getTrue();

    return cast<StackIDReleaseIntrinsic>(
        this->CreateCall2(BTDCall, stackID, Predicate));
}

// Note: 'traceRayCtrl' should be already by 8 bits to its location
// in the payload before passing as an argument to this function.
// getTraceRayPayload() just ORs together the bvh, ctrl, and stack id.
Value* RTBuilder::createTraceRay(
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

    auto* TraceRay = this->CreateCall(traceFn, Args);
    return TraceRay;
}

void RTBuilder::createReadSyncTraceRay(Value* val)
{
    Function* readFunc = GenISAIntrinsic::getDeclaration(
        this->GetInsertBlock()->getModule(),
        GenISAIntrinsic::GenISA_ReadTraceRaySync);

    this->CreateCall(readFunc, val);
}

Value* RTBuilder::createSyncTraceRay(
    Value* bvhLevel,
    Value* traceRayCtrl,
    const Twine& PayloadName)
{
    return createTraceRay(bvhLevel, this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true, PayloadName);
}

Value* RTBuilder::createSyncTraceRay(
    uint8_t bvhLevel,
    Value* traceRayCtrl,
    const Twine& PayloadName)
{
    return createTraceRay(this->getInt32(bvhLevel), this->CreateZExt(traceRayCtrl, this->getInt32Ty()), true, PayloadName);
}

Value* RTBuilder::createASyncTraceRay(
    Value* bvhLevel,
    unsigned int traceRayCtrl,
    const Twine& PayloadName)
{
    return createTraceRay(bvhLevel, this->getInt32(traceRayCtrl), false, PayloadName);
}

Value* RTBuilder::createASyncTraceRay(
    Value* bvhLevel,
    Value* traceRayCtrl,
    const Twine& PayloadName)
{
    return createTraceRay(bvhLevel, traceRayCtrl, false, PayloadName);
}

Value* RTBuilder::createASyncTraceRay(
    uint8_t bvhLevel,
    unsigned int traceRayCtrl,
    const Twine& PayloadName)
{
    return createTraceRay(this->getInt32(bvhLevel), this->getInt32(traceRayCtrl), false, PayloadName);
}

//FIXME: this is a temp solution and eventually, we want to have a general solution to coalesce this kind of store
//this method write a block of data (== size bytes) specified by vals into dstPtr.
//  If srcPtr is nullptr, all other data except vals are 0.
//  Else, all other data except vals are from srcPtr.
//size: mem size in bytes
//vals: key is byte offset of value. Only support DWORDs and i64 now.
void RTBuilder::WriteBlockData(Value* dstPtr, Value* srcPtr, uint32_t size, const DenseMap<uint32_t, Value*>& vals, const Twine& dstName, const Twine& srcName)
{
    //simply use DWORDs for now
    uint32_t nEles = size / 4;
    auto* VectorTy = IGCLLVM::FixedVectorType::get(getInt32Ty(), nEles);
    Value* blockDstPtr = CreateBitOrPointerCast(dstPtr, VectorTy->getPointerTo(dstPtr->getType()->getPointerAddressSpace()), dstName + ".BlockDataPtr");
    Value* blockSrc = nullptr;
    if (srcPtr)
    {
        Value* blockSrcPtr = CreateBitOrPointerCast(srcPtr, VectorTy->getPointerTo(srcPtr->getType()->getPointerAddressSpace()), srcName + ".BlockDataPtr");
        blockSrc = CreateAlignedLoad(blockSrcPtr, IGCLLVM::Align(size), srcName + ".BlockData");
    }

    Value* Vec = UndefValue::get(VectorTy);
    for (uint32_t i = 0; i < nEles; i++) {
        auto it = vals.find(i*4);
        if (it != vals.end())
        {
            Value* val = it->second;
            if (val->getType()->isIntegerTy(64))
            {
                Value* i32vec = CreateBitCast(val, IGCLLVM::FixedVectorType::get(getInt32Ty(), 2));
                Vec = CreateInsertElement(Vec, CreateExtractElement(i32vec, (uint64_t)0), i);
                i++;
                Vec = CreateInsertElement(Vec, CreateExtractElement(i32vec, (uint64_t)1), i);
            }
            else
            {
                IGC_ASSERT_MESSAGE(val->getType()->getScalarSizeInBits() == 32, "Not supported datatype!");
                Vec = CreateInsertElement(Vec, CreateBitCast(val, this->getInt32Ty()), i);
            }
        }
        else
        {
            if (blockSrc)
            {
                Vec = CreateInsertElement(Vec, CreateExtractElement(blockSrc, i), i);
            }
            else
            {
                Vec = CreateInsertElement(Vec, ConstantInt::get(this->getInt32Ty(), 0), i);
            }
        }
    }

    CreateAlignedStore(Vec, blockDstPtr, IGCLLVM::Align(size));
}

//If MemHit is invalid (below if case), workload might still try to access data with InstanceLeafPtr which is not set
//and is a dangling pointer. Even though this workload behavior is against API spec, but we should not "crash".
//To avoid this crash, we check if the data request is invalid here, if it's invalid, we return an invalid data (uint32: 0xFFFFFFFF, float: INF)
// if (forCommitted && committedHit::valid != 1) ||
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
std::pair<BasicBlock*, PHINode*> RTBuilder::validateInstanceLeafPtr(RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, bool forCommitted)
{
    Value* hitInfo = this->getHitInfoDWord(perLaneStackPtr,
        (forCommitted ? CallableShaderTypeMD::ClosestHit : CallableShaderTypeMD::AnyHit),
        VALUE_NAME("ValidDW"));

    Value* valid = nullptr;
    if (forCommitted)
    {
        valid = CreateAnd(
            hitInfo,
            getInt32(BIT((uint32_t)MemHit::Offset::valid)));
        valid = CreateICmpNE(valid, this->getInt32(0), VALUE_NAME("valid"));
    }
    else
    {
        Value* leafType = this->CreateLShr(
            hitInfo, this->getInt32((uint32_t)MemHit::Offset::leafType));

        leafType = this->CreateAnd(
            leafType,
            this->getInt32(BITMASK((uint32_t)MemHit::Bits::leafType)),
            VALUE_NAME("leafType"));

        valid = this->CreateICmpNE(leafType, this->getInt32(NODE_TYPE_INVALID), VALUE_NAME("validLeafType"));
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
    {
        invalidVal = this->getInt32(IGC_GET_FLAG_VALUE(RTInValidDefaultIndex));
    }
    else if (I->getType()->isFloatTy())
    {
        invalidVal = ConstantFP::getInfinity(this->getFloatTy());
    }
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
    auto& M = *this->GetInsertBlock()->getModule();
    //FIXME: Temp solution: to shrink ShadowMemory, if ShrinkShadowMemoryIfNoSpillfill is true (we know no spillfill), then,
    //we alloca SMStack instead of RTStack to reduce the size. Also, here, we simply cast SMStack pointer to RTStack which is risky
    //and it's safe now if we only shrink the last variable of RTStack (MemTravStack).
    //But don't shrink data in the middle of RTStack which will lead to holes.
    auto* RTStackPtrTy = bShrinkSMStack ? _gettype_SMStack2(M) : _gettype_RTStack2(M);

    Value* rtStacks = this->CreateAlloca(
        ArrayType::get(RTStackPtrTy, size),
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

void RTBuilder::SpillRayQueryShadowMemory(Value* dst, Value* shMem, uint64_t size, unsigned align)
{
    this->CreateMemCpy(
        dst,
        this->CreatePointerCast(shMem, PointerType::get(this->getInt32Ty(), ADDRESS_SPACE_PRIVATE)),
        size,
        align);
}

void RTBuilder::FillRayQueryShadowMemory(Value* shMem, Value* src, uint64_t size, unsigned align)
{
    this->CreateMemCpy(
        this->CreatePointerCast(shMem, PointerType::get(this->getInt32Ty(), ADDRESS_SPACE_PRIVATE)),
        src,
        size,
        align);
}

Value* RTBuilder::LoadInstanceContributionToHitGroupIndex(Value* instLeafPtr)
{
    uint32_t offset = offsetof(InstanceLeaf, part0.instContToHitGroupIndex);
    Value* offsetVal = this->CreateAdd(instLeafPtr, this->getInt64(offset));
    Value* valuePtr = this->CreateIntToPtr(offsetVal, PointerType::get(this->getInt32Ty(), ADDRESS_SPACE_GLOBAL));
    Value* value = this->CreateLoad(valuePtr);

    //Only lower 24bits for the Dword read are valid for Index
    return this->CreateAnd(value, this->getInt32(BITMASK((uint32_t)InstanceLeaf::Part0::Bits::instContToHitGrpIndex)));
}

void RTBuilder::MemCpyPotentialHit2CommitHit(RTBuilder::StackPointerVal* StackPointer)
{
    Value* committedHit = this->_gepof_CommittedHitT(StackPointer, VALUE_NAME("&committedHit"));
    Value* potentialHit = this->_gepof_PotentialHitT(StackPointer, VALUE_NAME("&potentialHit"));

    this->CreateMemCpy(
        committedHit,
        4,
        potentialHit,
        4,
        this->getInt64(sizeof(MemHit)));
}

void RTBuilder::setPotentialDoneBit(RTBuilder::StackPointerVal* StackPointer)
{
    Value* doneDW = this->getPotentialHitInfo(StackPointer, VALUE_NAME("DoneDW"));
    Value* newHitInfo = this->CreateOr(
        doneDW, this->getInt32(BIT((uint32_t)MemHit::Offset::done)));
    this->setHitInfoDWord(StackPointer, CallableShaderTypeMD::AnyHit, newHitInfo, VALUE_NAME("DoneDW"));
}

void RTBuilder::CreateAbort(RTBuilder::StackPointerVal* StackPointer)
{
    setPotentialDoneBit(StackPointer);
}

uint32_t RTBuilder::getSWHotZoneSize()
{
    return IGC_IS_FLAG_ENABLED(EnableCompressedRayIndices) ?
        sizeof(SWHotZone_v1) :
        sizeof(SWHotZone_v2);
}

Value* RTBuilder::getNumAsyncStackSlots()
{
    // NumSlots = DSS_COUNT * NumDSSRTStacks
    Value* NumSlots = this->CreateMul(
        this->getpNumDSSRTStacks(),
        this->getInt32(MaxDualSubSlicesSupported));
    return NumSlots;
}

// These computations should match RTStackFormat::calcRTMemoryAllocSize().
Value* RTBuilder::getSumSWHotZoneSize()
{
    // Align(SWHotZoneSize * DSS_COUNT * NumDSSRTStacks, RTStackAlign)
    Value* Size = this->CreateMul(
        this->getInt32(getSWHotZoneSize()),
        this->getNumAsyncStackSlots());
    Size = this->alignVal(Size, RTStackAlign);
    return Size;
}

uint32_t RTBuilder::getSyncStackSize()
{
    return Ctx.getModuleMetaData()->rtInfo.RayQueryAllocSizeInBytes;
}

uint32_t RTBuilder::getNumSyncStackSlots()
{
    return MaxDualSubSlicesSupported * Ctx.platform.getRTStackDSSMultiplier();
}

uint32_t RTBuilder::getSumSyncStackSize()
{
    // Align(SyncStackSize * DSS_COUNT * SIMD_LANES_PER_DSS, RTStackAlign)
    uint32_t Val = IGC::Align(getSyncStackSize()* getNumSyncStackSlots(), RTStackAlign);
    return Val;
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

Value* RTBuilder::getRayInfoPtr(
    StackPointerVal* StackPointer, uint32_t Idx, uint32_t BvhLevel)
{
    IGC_ASSERT_MESSAGE((Idx < 8), "out of bounds!");

    static_assert(offsetof(HWRayData2, ray[TOP_LEVEL_BVH].dir) ==
                  offsetof(HWRayData2, ray[TOP_LEVEL_BVH].org) + sizeof(Vec3f),
        "layout change?");
    static_assert(offsetof(HWRayData2, ray[TOP_LEVEL_BVH].tnear) ==
                  offsetof(HWRayData2, ray[TOP_LEVEL_BVH].dir) + sizeof(Vec3f),
        "layout change?");
    static_assert(offsetof(HWRayData2, ray[TOP_LEVEL_BVH].tfar) ==
                  offsetof(HWRayData2, ray[TOP_LEVEL_BVH].tnear) + sizeof(float),
        "layout change?");

    Value* Ptr = nullptr;
    if (Idx < 3)
    {
        Ptr = _gepof_MemRay_org(StackPointer,
            this->getInt32(BvhLevel), this->getInt32(Idx),
            VALUE_NAME("&MemRay::org[" + Twine(Idx) + "]"));
    }
    else if (Idx < 6)
    {
        Ptr = _gepof_MemRay_dir(StackPointer,
            this->getInt32(BvhLevel), this->getInt32(Idx - 3),
            VALUE_NAME("&MemRay::dir[" + Twine(Idx - 3) + "]"));
    }
    else if (Idx == 6)
    {
        Ptr = _gepof_MemRay_tnear(StackPointer,
            this->getInt32(BvhLevel),
            VALUE_NAME("&MemRay::tnear"));
    }
    else if (Idx == 7)
    {
        Ptr = _gepof_MemRay_tfar(StackPointer,
            this->getInt32(BvhLevel),
            VALUE_NAME("&MemRay::tfar"));
    }

    return Ptr;
}

Value* RTBuilder::getRayInfo(StackPointerVal* perLaneStackPtr, uint32_t Idx)
{
    Value* Ptr = this->getRayInfoPtr(perLaneStackPtr, Idx, TOP_LEVEL_BVH);
    Value* info = this->CreateLoad(Ptr, VALUE_NAME("RayInfo." + Twine(Idx)));
    return info;
}

Value* RTBuilder::getRayTMin(RTBuilder::StackPointerVal* perLaneStackPtr)
{
    Value* Ptr = this->_gepof_MemRay_tnear(
        perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH),
        VALUE_NAME("&MemRay::tnear"));
    return this->CreateLoad(Ptr, VALUE_NAME("rayTMin"));
}

Value* RTBuilder::getRayFlagsPtr(RTBuilder::SyncStackPointerVal* perLaneStackPtr)
{
    auto* Ptr = this->_gepof_topOfNodePtrAndFlags(
        perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH));
    Ptr = this->CreateBitCast(
        Ptr,
        PointerType::get(
            this->getInt16Ty(), Ptr->getType()->getPointerAddressSpace()));
    static_assert((uint32_t)MemRay::Bits::rootNodePtr == 48, "Changed?");
    static_assert((uint32_t)MemRay::Bits::rayFlags == 16, "Changed?");
    Value* Indices[] = { this->getInt32(3) };
    return this->CreateInBoundsGEP(Ptr, Indices, VALUE_NAME("&rayFlags"));
}

// returns an i16 value containing the current rayflags. Sync only.
Value* RTBuilder::getRayFlags(RTBuilder::SyncStackPointerVal* perLaneStackPtr)
{
    return this->CreateLoad(getRayFlagsPtr(perLaneStackPtr), VALUE_NAME("rayflags"));
}

void RTBuilder::setRayFlags(RTBuilder::SyncStackPointerVal* perLaneStackPtr, Value* V)
{
    this->CreateStore(V, this->getRayFlagsPtr(perLaneStackPtr));
}

Value* RTBuilder::getRayFlagsPtr(RTBuilder::AsyncStackPointerVal* perLaneStackPtr)
{
    auto* Ptr = this->_gepof_topOfInstanceLeafPtr(
        perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH));
    Ptr = this->CreateBitCast(
        Ptr,
        PointerType::get(
            this->getInt16Ty(), Ptr->getType()->getPointerAddressSpace()),
        VALUE_NAME("&rayFlags"));
    static_assert((uint32_t)MemRay::Offset::rayFlagsCopy == 0, "Changed?");
    static_assert((uint32_t)MemRay::Bits::rayFlagsCopy == 16, "Changed?");

    return Ptr;
}

// returns an i16 value containing the current rayflags. Async only.
Value* RTBuilder::getRayFlags(RTBuilder::AsyncStackPointerVal* perLaneStackPtr)
{
    return this->CreateLoad(this->getRayFlagsPtr(perLaneStackPtr), VALUE_NAME("rayFlags"));
}

Value* RTBuilder::getWorldRayOrig(RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim)
{
    Value* Ptr = this->_gepof_MemRay_org(
        perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH), this->getInt32(dim),
        VALUE_NAME("&MemRay::org[" + Twine(dim) + "]"));
    Value* info = this->CreateLoad(Ptr,
        VALUE_NAME("WorldRayOrig[" + Twine(dim) + "]"));
    return info;
}

Value* RTBuilder::getWorldRayDir(RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t dim)
{
    Value* Ptr = this->_gepof_MemRay_dir(
        perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH), this->getInt32(dim),
        VALUE_NAME("&MemRay::dir[" + Twine(dim) + "]"));
    Value* info = this->CreateLoad(Ptr,
        VALUE_NAME("WorldRayDir[" + Twine(dim) + "]"));
    return info;
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
        Value* Ptr = this->_gepof_MemRay_org(
            perLaneStackPtr, this->getInt32(BOTTOM_LEVEL_BVH), this->getInt32(dim),
            VALUE_NAME("&MemRay::org[" + Twine(dim) + "]"));
        info = this->CreateLoad(Ptr,
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
        Value* Ptr = this->_gepof_MemRay_dir(
            perLaneStackPtr, this->getInt32(BOTTOM_LEVEL_BVH), this->getInt32(dim),
            VALUE_NAME("&MemRay::dir[" + Twine(dim) + "]"));
        info = this->CreateLoad(Ptr,
            VALUE_NAME("ObjRayDir[" + Twine(dim) + "]"));
    }
    return info;
}

Value* RTBuilder::getRayTCurrent(
    RTBuilder::StackPointerVal* perLaneStackPtr, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* Ptr = nullptr;
    if (ShaderTy == CallableShaderTypeMD::AnyHit ||
        ShaderTy == CallableShaderTypeMD::Intersection)
    {
        Ptr = this->_gepof_PotentialHitT(perLaneStackPtr,
            VALUE_NAME("&potentialHit.t"));
    }
    else if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        Ptr = this->_gepof_CommittedHitT(perLaneStackPtr,
            VALUE_NAME("&committedHit.t"));
    }
    else if (ShaderTy == CallableShaderTypeMD::Miss)
    {
        Ptr = _gepof_MemRay_tfar(perLaneStackPtr,
            this->getInt32(0),
            VALUE_NAME("&MemRay::tfar"));
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "RayTCurrent() not supported in this shader!");
    }

    Value* info = this->CreateLoad(Ptr);
    return info;
}

Value* RTBuilder::getInstance(
    RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t infoKind, IGC::CallableShaderTypeMD ShaderTy,
    Instruction* I, bool checkInstanceLeafPtr)
{
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr))
    {
        std::pair<BasicBlock*, PHINode*> pair = validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(&*pair.first->rbegin());
        Value* validVal = getInstance(perLaneStackPtr, infoKind, ShaderTy);
        this->SetInsertPoint(I);
        pair.second->addIncoming(validVal, pair.first);
        return pair.second;
    }
    else
    {
        return getInstance(perLaneStackPtr, infoKind, ShaderTy);
    }
}

Value* RTBuilder::getInstance(
    RTBuilder::StackPointerVal* perLaneStackPtr, uint32_t infoKind, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* instLeafTopPtr = nullptr;

    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        instLeafTopPtr = this->_gepof_CommittedHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&committedHit.topOfInstLeafPtr"));
    }
    else
    {
        instLeafTopPtr = this->_gepof_PotentialHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&potentialHit.topOfInstLeafPtr"));
    }

    //Read Instance Leaf Ptr
    Value* Ptr = this->getInstanceLeafPtr(instLeafTopPtr);

    Value* InfoPtr = (infoKind == INSTANCE_ID) ?
        this->_gepof_InstanceLeaf_instanceID(Ptr,
            VALUE_NAME("&InstanceLeaf...instanceID")) :
        this->_gepof_InstanceLeaf_instanceIndex(Ptr,
            VALUE_NAME("&InstanceLeaf...instanceIndex"));

    Value* info = this->CreateLoad(InfoPtr,
        VALUE_NAME((infoKind == INSTANCE_ID) ? "instanceID" : "instanceIndex"));
    return info;
}

Value* RTBuilder::getPrimitiveIndex(RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy, bool checkInstanceLeafPtr)
{
    uint32_t mask = (ShaderTy == CallableShaderTypeMD::ClosestHit ? 1 : 2);
    bool bMask = (IGC_GET_FLAG_VALUE(ForceRTCheckInstanceLeafPtrMask) & mask);
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr) && bMask)
    {
        std::pair<BasicBlock*, PHINode*> pair = validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(&*pair.first->rbegin());
        PHINode* validPrimIndex = getPrimitiveIndex(perLaneStackPtr, &*pair.first->rbegin(), infoKind, ShaderTy);
        this->SetInsertPoint(I);
        pair.second->addIncoming(validPrimIndex, validPrimIndex->getParent());
        return pair.second;
    }
    else
    {
        return getPrimitiveIndex(perLaneStackPtr, I, infoKind, ShaderTy);
    }
}

PHINode* RTBuilder::getPrimitiveIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy)
{
    //Read Primitive Leaf Ptr
    Value* loadValue = this->getHitTopOfPrimLeafPtr(perLaneStackPtr, ShaderTy);
    //Get lower 42 Bits.
    loadValue = this->CreateAnd(loadValue,
        this->getInt64(QWBITMASK((uint32_t)MemHit::Bits::primLeafPtr)),
        VALUE_NAME("42-bit PrimLeafPtr"));
    Value* fullPrimLeafPtr = this->CreateMul(loadValue, this->getInt64(LeafSize),
        VALUE_NAME("fullPrimLeafPtr"));
    fullPrimLeafPtr = canonizePointer(fullPrimLeafPtr);

    auto& C = I->getContext();
    auto* CSBlock = I->getParent();
    auto* endBlock = CSBlock->splitBasicBlock(I, VALUE_NAME("primitiveIndexEndBlock"));
    Function* F = this->GetInsertBlock()->getParent();

    BasicBlock* quadMeshLeafBB = BasicBlock::Create(C, VALUE_NAME("QuadMeshLeafBB"), F, endBlock);
    BasicBlock* prodeduralLeafBB = BasicBlock::Create(C, VALUE_NAME("ProduralLeafBB"), F, endBlock);
    CSBlock->getTerminator()->eraseFromParent();
    this->SetInsertPoint(CSBlock);

    Value* primLeafIndexTop = this->getHitInfoDWord(perLaneStackPtr, ShaderTy, VALUE_NAME("topOfPrimIndexDelta"));
    Value* leafTyCmp = this->CreateICmpEQ(infoKind, this->getInt32(NODE_TYPE_PROCEDURAL));
    this->CreateCondBr(leafTyCmp, prodeduralLeafBB, quadMeshLeafBB);
    auto& M = *this->GetInsertBlock()->getModule();

    //QuadLeaf: leafTyCmp == false
    Value* quadPrimIndex = nullptr;
    {
        this->SetInsertPoint(quadMeshLeafBB);
        Value* quadPrimLeafPtr = this->CreateIntToPtr(
            fullPrimLeafPtr, this->getQuadLeafPtrTy(M),
            VALUE_NAME("QuadLeafPtr"));
        Value* primIndex0Ptr = this->_gepof_QuadLeaf_primIndex0(
            quadPrimLeafPtr, VALUE_NAME("&QuadLeaf::primIndex0"));
        Value* primIndex0 = this->CreateLoad(primIndex0Ptr, VALUE_NAME("primIndex0"));

        Value* primLeafIndexDelta = this->CreateAnd(primLeafIndexTop,
            this->getInt32(BITMASK((uint32_t)MemHit::Bits::primIndexDelta)),
            VALUE_NAME("primLeafIndexDelta"));
        quadPrimIndex = this->CreateAdd(primLeafIndexDelta, primIndex0, VALUE_NAME("primIndex"));
        this->CreateBr(endBlock);
    }

    //ProdecuralLeaf: leafTyCmp == true
    Value* proceduralPrimIndex = nullptr;
    {
        this->SetInsertPoint(prodeduralLeafBB);
        Value* primLeafIndex = this->CreateLShr(primLeafIndexTop,
            this->getInt32((uint32_t)MemHit::Offset::primLeafIndex));
        primLeafIndex = this->CreateAnd(primLeafIndex,
            this->getInt32(BITMASK((uint32_t)MemHit::Bits::primLeafIndex)),
            VALUE_NAME("primLeafIndex"));

        Value* procPrimLeafPtr = this->CreateIntToPtr(
            fullPrimLeafPtr, this->getProceduralLeafPtrTy(M),
            VALUE_NAME("ProceduralLeafPtr"));
        Value* primIndexPtr = this->_gepof_ProceduralLeaf__primIndex(
            procPrimLeafPtr, primLeafIndex,
            VALUE_NAME("&ProceduralLeaf._primIndex[i]"));
        proceduralPrimIndex = this->CreateLoad(primIndexPtr, VALUE_NAME("primIndex"));
        this->CreateBr(endBlock);
    }

    //END block
    this->SetInsertPoint(I);
    PHINode* phi = this->CreatePHI(quadPrimIndex->getType(), 2);
    phi->addIncoming(quadPrimIndex, quadMeshLeafBB);
    phi->addIncoming(proceduralPrimIndex, prodeduralLeafBB);

    return phi;
}

Value* RTBuilder::getGeometryIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy, bool checkInstanceLeafPtr)
{
    uint32_t mask = (ShaderTy == CallableShaderTypeMD::ClosestHit ? 1 : 2);
    bool bMask = (IGC_GET_FLAG_VALUE(ForceRTCheckInstanceLeafPtrMask) & mask);
    if (checkInstanceLeafPtr && IGC_IS_FLAG_ENABLED(ForceRTCheckInstanceLeafPtr) && bMask)
    {
        std::pair<BasicBlock*, PHINode*> pair = validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(&*pair.first->rbegin());
        std::pair<Value*, BasicBlock*> validGeomIndexPair = getGeometryIndex(perLaneStackPtr, &*pair.first->rbegin(), infoKind, ShaderTy);
        this->SetInsertPoint(I);
        pair.second->addIncoming(validGeomIndexPair.first, validGeomIndexPair.second);
        return pair.second;
    }
    else
    {
        return getGeometryIndex(perLaneStackPtr, I, infoKind, ShaderTy).first;
    }
}

std::pair<Value*, BasicBlock*> RTBuilder::getGeometryIndex(
    RTBuilder::StackPointerVal* perLaneStackPtr, Instruction* I, Value* infoKind, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* loadValue = this->getHitTopOfPrimLeafPtr(perLaneStackPtr, ShaderTy);

    // Extract 42 lower bits to get primitive leaf node offset
    loadValue = this->CreateAnd(loadValue,
        this->getInt64(QWBITMASK((uint32_t)MemHit::Bits::primLeafPtr)),
        VALUE_NAME("42-bit PrimLeafPtr"));
    // Multiply by size of primitive leaf node
    Value* fullPrimLeafPtr = this->CreateMul(loadValue, this->getInt64(LeafSize),
        VALUE_NAME("fullPrimLeafPtr"));

    fullPrimLeafPtr = canonizePointer(fullPrimLeafPtr);

    auto& C = I->getContext();
    auto* CSBlock = I->getParent();
    auto* endBlock = CSBlock->splitBasicBlock(I, VALUE_NAME("geometryIndexEndBlock"));
    Function* F = this->GetInsertBlock()->getParent();

    BasicBlock* quadMeshLeafBB = BasicBlock::Create(C, VALUE_NAME("QuadMeshLeafBB"), F, endBlock);
    BasicBlock* prodeduralLeafBB = BasicBlock::Create(C, VALUE_NAME("ProduralLeafBB"), F, endBlock);
    CSBlock->getTerminator()->eraseFromParent();
    this->SetInsertPoint(CSBlock);
    Value* leafTyCmp = this->CreateICmpEQ(infoKind, this->getInt32(NODE_TYPE_PROCEDURAL));
    this->CreateCondBr(leafTyCmp, prodeduralLeafBB, quadMeshLeafBB);
    auto& M = *this->GetInsertBlock()->getModule();

    //QuadLeaf: leafTyCmp == false
    Value* quadGeometryIndex = nullptr;
    {
        this->SetInsertPoint(quadMeshLeafBB);
        Value* quadPrimLeafPtr = this->CreateIntToPtr(
            fullPrimLeafPtr, this->getQuadLeafPtrTy(M),
            VALUE_NAME("QuadLeafPtr"));

        Value* quadGeometryIndexPtr = this->_gepof_QuadLeaf_topOfGeomIndex(
            quadPrimLeafPtr, VALUE_NAME("&QuadLeaf...topOfGeomIndex"));
        quadGeometryIndex = this->CreateLoad(quadGeometryIndexPtr, VALUE_NAME("topOfGeomIndex"));
        this->CreateBr(endBlock);
    }

    //ProdecuralLeaf: leafTyCmp == true
    Value* proceduralGeometryIndex = nullptr;
    {
        this->SetInsertPoint(prodeduralLeafBB);
        Value* procPrimLeafPtr = this->CreateIntToPtr(
            fullPrimLeafPtr, this->getProceduralLeafPtrTy(M),
            VALUE_NAME("ProceduralLeafPtr"));

        Value* proceduralGeometryIndexPtr = this->_gepof_ProceduralLeaf_topOfGeomIndex(
            procPrimLeafPtr, VALUE_NAME("&ProceduralLeaf...topOfGeomIndex"));
        proceduralGeometryIndex = this->CreateLoad(proceduralGeometryIndexPtr, VALUE_NAME("topOfGeomIndex"));
        this->CreateBr(endBlock);
    }

    //END block
    this->SetInsertPoint(I);
    PHINode* phi = this->CreatePHI(quadGeometryIndex->getType(), 2);
    phi->addIncoming(quadGeometryIndex, quadMeshLeafBB);
    phi->addIncoming(proceduralGeometryIndex, prodeduralLeafBB);

    // Extract 29 lower bits to get geometry index
    Value* info = this->CreateAnd(phi,
        this->getInt32(BITMASK((uint32_t)PrimLeafDesc::Bits::geomIndex)),
        VALUE_NAME("geomIndex"));

    return std::make_pair(info, endBlock);
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

Value* RTBuilder::getCommittedHitPtr(RTBuilder::StackPointerVal* perLaneStackPtr)
{
    static_assert(offsetof(MemHit, t) == 0);
    return this->_gepof_CommittedHitT(perLaneStackPtr,
        VALUE_NAME("&CommittedHit"));
}

Value* RTBuilder::getPotentialHitPtr(RTBuilder::StackPointerVal* perLaneStackPtr)
{
    static_assert(offsetof(MemHit, t) == 0);
    return this->_gepof_PotentialHitT(perLaneStackPtr,
        VALUE_NAME("&PotentialHit"));
}

Value* RTBuilder::getMemRayPtr(RTBuilder::StackPointerVal* perLaneStackPtr, bool isTopLevel)
{
    static_assert(offsetof(MemRay, org) == 0);
    return this->_gepof_MemRay_org(
        perLaneStackPtr, this->getInt32(isTopLevel ? TOP_LEVEL_BVH : BOTTOM_LEVEL_BVH), this->getInt32(0),
        VALUE_NAME("&MemRay"));
}

Value* RTBuilder::getMatrixPtr(
    Value* InstanceLeafPtr,
    uint32_t infoKind,
    uint32_t dim)
{
    IGC_ASSERT_MESSAGE((dim < 12), "dim out of bounds!");

    static_assert(
        offsetof(InstanceLeaf, part1.obj2world_vy) ==
        offsetof(InstanceLeaf, part1.obj2world_vx) + sizeof(Vec3f),
        "layout change?");
    static_assert(
        offsetof(InstanceLeaf, part1.obj2world_vz) ==
        offsetof(InstanceLeaf, part1.obj2world_vy) + sizeof(Vec3f),
        "layout change?");
    static_assert(
        offsetof(InstanceLeaf, part1.world2obj_p) ==
        offsetof(InstanceLeaf, part1.obj2world_vz) + sizeof(Vec3f),
        "layout change?");

    static_assert(
        offsetof(InstanceLeaf, part0.world2obj_vy) ==
        offsetof(InstanceLeaf, part0.world2obj_vx) + sizeof(Vec3f),
        "layout change?");
    static_assert(
        offsetof(InstanceLeaf, part0.world2obj_vz) ==
        offsetof(InstanceLeaf, part0.world2obj_vy) + sizeof(Vec3f),
        "layout change?");
    static_assert(
        offsetof(InstanceLeaf, part0.obj2world_p) ==
        offsetof(InstanceLeaf, part0.world2obj_vz) + sizeof(Vec3f),
        "layout change?");

    IGC_ASSERT_MESSAGE((infoKind == OBJECT_TO_WORLD || infoKind == WORLD_TO_OBJECT), "wrong kind!");

    //3x3 matrix is represented in the bvh as a 3 float3 vectors, each representing a row.
    Value* componentPtr = nullptr;
    if (infoKind == OBJECT_TO_WORLD)
    {
        if (dim < 3)
        {
            componentPtr = this->_gepof_InstanceLeaf_obj2world_vx(
                InstanceLeafPtr, this->getInt32(dim),
                VALUE_NAME("&InstanceLeaf...obj2world_vx[" + Twine(dim) + "]"));
        }
        else if (dim < 6)
        {
            componentPtr = this->_gepof_InstanceLeaf_obj2world_vy(
                InstanceLeafPtr, this->getInt32(dim - 3),
                VALUE_NAME("&InstanceLeaf...obj2world_vy[" + Twine(dim - 3) + "]"));
        }
        else if (dim < 9)
        {
            componentPtr = this->_gepof_InstanceLeaf_obj2world_vz(
                InstanceLeafPtr, this->getInt32(dim - 6),
                VALUE_NAME("&InstanceLeaf...obj2world_vz[" + Twine(dim - 6) + "]"));
        }
        else
        {
            componentPtr = this->_gepof_InstanceLeaf_obj2world_p(
                InstanceLeafPtr, this->getInt32(dim - 9),
                VALUE_NAME("&InstanceLeaf...obj2world_p[" + Twine(dim - 9) + "]"));
        }
    }
    else
    {
        if (dim < 3)
        {
            componentPtr = this->_gepof_InstanceLeaf_world2obj_vx(
                InstanceLeafPtr, this->getInt32(dim),
                VALUE_NAME("&InstanceLeaf...world2obj_vx[" + Twine(dim) + "]"));
        }
        else if (dim < 6)
        {
            componentPtr = this->_gepof_InstanceLeaf_world2obj_vy(
                InstanceLeafPtr, this->getInt32(dim - 3),
                VALUE_NAME("&InstanceLeaf...world2obj_vy[" + Twine(dim - 3) + "]"));
        }
        else if (dim < 9)
        {
            componentPtr = this->_gepof_InstanceLeaf_world2obj_vz(
                InstanceLeafPtr, this->getInt32(dim - 6),
                VALUE_NAME("&InstanceLeaf...world2obj_vz[" + Twine(dim - 6) + "]"));
        }
        else
        {
            componentPtr = this->_gepof_InstanceLeaf_world2obj_p(
                InstanceLeafPtr, this->getInt32(dim - 9),
                VALUE_NAME("&InstanceLeaf...world2obj_p[" + Twine(dim - 9) + "]"));
        }
    }

    return componentPtr;
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
        std::pair<BasicBlock*, PHINode*> pair = validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(&*pair.first->rbegin());
        Value* validVal = getObjWorldAndWorldObj(perLaneStackPtr, infoKind, dim, ShaderTy);
        this->SetInsertPoint(I);
        pair.second->addIncoming(validVal, pair.first);
        return pair.second;
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
    Value* instanceLeafOffsetPtr = nullptr;

    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        instanceLeafOffsetPtr = this->_gepof_CommittedHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&committedHit.topOfInstLeafPtr"));
    }
    else if (ShaderTy == CallableShaderTypeMD::AnyHit ||
        ShaderTy == CallableShaderTypeMD::Intersection)
    {
        instanceLeafOffsetPtr = this->_gepof_PotentialHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&potentialHit.topOfInstLeafPtr"));
    }

    //Read Instance Leaf Ptr
    Value* ptrValue = this->getInstanceLeafPtr(instanceLeafOffsetPtr);

    Value* componentPtr = this->getMatrixPtr(ptrValue, infoKind, dim);

    Value* info = this->CreateLoad(
        componentPtr, VALUE_NAME("matrix[" + Twine(dim) + "]"));
    return info;
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
        std::pair<BasicBlock*, PHINode*> pair = validateInstanceLeafPtr(perLaneStackPtr, I, (ShaderTy == CallableShaderTypeMD::ClosestHit));
        this->SetInsertPoint(&*pair.first->rbegin());
        Value* validVal = TransformWorldToObject(perLaneStackPtr, dim, isOrigin, ShaderTy);
        this->SetInsertPoint(I);
        pair.second->addIncoming(validVal, pair.first);
        return pair.second;
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

    Value* instLeafTopPtr;
    {
        instLeafTopPtr = this->_gepof_CommittedHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&committedHit.topOfInstLeafPtr"));
    }

    //Read Instance Leaf Ptr
    Value* ptrValue = this->getInstanceLeafPtr(instLeafTopPtr);
    Value* worldRayPtr = nullptr;
    Value* acc = nullptr;
    if (isOrigin)
    {
        //Get pointer to the World Ray Origin x,y,z
        worldRayPtr = this->_gepof_MemRay_org(
            perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH), this->getInt32(0),
            VALUE_NAME("&MemRay::org[0]"));

        Value* transCompPtr = this->_gepof_InstanceLeaf_world2obj_p(
            ptrValue, this->getInt32(dim),
            VALUE_NAME("&InstanceLeaf...world2obj_p[" + Twine(dim) + "]"));
        //ray Origin Handling - Origin is a point, and points are affected by translation component of matrix
        acc = this->CreateLoad(transCompPtr, VALUE_NAME("transComp"));
    }
    else
    {
        worldRayPtr = this->_gepof_MemRay_dir(
            perLaneStackPtr, this->getInt32(TOP_LEVEL_BVH), this->getInt32(0),
            VALUE_NAME("MemRay::dir[0]"));
        //Ray Direction handling - Direction is a Vector, and vectors are not affected by
        //translation component of matrix. Set accumulator to zero to start
        acc = ConstantFP::get(this->getFloatTy(), 0.0);
    }

    uint32_t AddrSpace = perLaneStackPtr->getType()->getPointerAddressSpace();
    worldRayPtr = this->CreateBitCast(
        worldRayPtr,
        PointerType::get(IGCLLVM::FixedVectorType::get(this->getFloatTy(), 3), AddrSpace));
    Value* rayInfo = this->CreateLoad(worldRayPtr, VALUE_NAME("rayInfo"));

    //Do the math. Dot 4
    for (unsigned int i = 0; i < 3; i++)
    {
        Value* Ptr = this->getMatrixPtr(ptrValue, WORLD_TO_OBJECT, i * 3 + dim);
        Value* currentComp = this->CreateLoad(Ptr, VALUE_NAME("matrixComp"));

        Value* worldRayComp = this->CreateExtractElement(rayInfo, this->getInt32(i));
        Value* compMul = this->CreateFMul(currentComp, worldRayComp);

        acc = this->CreateFAdd(acc, compMul, VALUE_NAME("WorldToObject"));
    }

    return acc;
}

Value* RTBuilder::getInstanceLeafPtr(Value* instLeafTopPtr)
{
    auto& M = *this->GetInsertBlock()->getModule();
    //Read Instance Leaf Ptr
    Value* loadValue = this->CreateLoad(instLeafTopPtr);

    loadValue = this->CreateAnd(loadValue,
        this->getInt64(QWBITMASK((uint32_t)MemHit::Bits::instLeafPtr)),
        VALUE_NAME("42-bit InstanceLeafPtr"));
    Value* ptrValue = this->CreateMul(loadValue, this->getInt64(LeafSize),
        VALUE_NAME("fullInstanceLeafPtr"));
    ptrValue = canonizePointer(ptrValue);
    ptrValue = this->CreateIntToPtr(ptrValue, this->getInstanceLeafPtrTy(M),
        VALUE_NAME("InstanceLeafPtr"));

    return ptrValue;
}

// Both input values come in as i64 values.  Each has 22 active bits
// and they are in the low 22 bits of the value.
// The final address is computed as:
// uint64_t hitGroupRecPtr;
// hitGroupRecPtr = uint64_t(hitGroupRecPtr0);
// hitGroupRecPtr |= uint64_t(hitGroupRecPtr1) << 22;
// return 16 * hitGroupRecPtr;
Value* RTBuilder::getHitGroupRecPtr(
    Value* hitGroupRecPtr0, Value* hitGroupRecPtr1)
{
    auto* Combo = this->CreateOr(
        hitGroupRecPtr0,
        this->CreateShl(
            hitGroupRecPtr1,
            (uint32_t)MemHit::Bits::hitGroupRecPtr0));
    return this->canonizePointer(CreateShl(Combo, 4, VALUE_NAME("hitGroupRecPtr")));
}

Value* RTBuilder::getHitGroupRecPtrFromPrimAndInstVals(
    Value* PotentialPrimVal, Value* PotentialInstVal)
{
    // hitGroupRecPtr0 and hitGroupRecPtr1 from potentialHit have already
    // been populated with the address prior to the start of the shader.
    Value* hitGroupRecPtr0 = this->CreateLShr(
        PotentialPrimVal,
        (uint32_t)MemHit::Bits::primLeafPtr);
    Value* hitGroupRecPtr1 = this->CreateLShr(
        PotentialInstVal,
        (uint32_t)MemHit::Bits::instLeafPtr);

    Value* ClosestHitAddress = this->getHitGroupRecPtr(
        hitGroupRecPtr0, hitGroupRecPtr1);

    return ClosestHitAddress;
}

Value* RTBuilder::CreateShaderType()
{
    auto* M = this->GetInsertBlock()->getModule();
    Function* Func = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_DCL_SystemValue,
        this->getInt32Ty());

    return this->CreateCall(
        Func, this->getInt32(IGC::SHADER_TYPE), VALUE_NAME("shader_type"));
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
    else
    {
        stackId = this->getAsyncStackID();
        stackId = this->CreateZExt(stackId, this->getInt32Ty());
    }

    constexpr uint16_t StackIDOffset =
        offsetof(TraceRayMessage::Payload, stackID) * 8;
    Value* bitField = this->CreateOr(
        traceRayCtrl,
        this->CreateShl(stackId, StackIDOffset));
    bitField = this->CreateOr(bitField, bvhLevel, PayloadName);
    return bitField;
}

Value* RTBuilder::emitStateRegID(uint32_t BitStart, uint32_t BitEnd)
{
    Module* module = this->GetInsertBlock()->getModule();

    Value* sr0_0 = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            module, GenISAIntrinsic::GenISA_getSR0_0),
        None,
        VALUE_NAME("sr0.0"));

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

std::pair<uint32_t, uint32_t> RTBuilder::getSubsliceIDBitsInSR0() const {
    if (Ctx.platform.GetPlatformFamily() == IGFX_GEN8_CORE ||
        Ctx.platform.GetPlatformFamily() == IGFX_GEN9_CORE)
    {
        return {12, 13};
    }
    else
    {
        return {8, 8};
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

Value* RTBuilder::getSliceID()
{
    auto bitsInSR0 = getSliceIDBitsInSR0();
    return emitStateRegID(bitsInSR0.first, bitsInSR0.second);
}

Value* RTBuilder::getSubsliceID()
{
    auto bitsInSR0 = getSubsliceIDBitsInSR0();
    return emitStateRegID(bitsInSR0.first, bitsInSR0.second);
}

Value* RTBuilder::getDualSubsliceID()
{
    auto bitsInSR0 = getDualSubsliceIDBitsInSR0();
    return emitStateRegID(bitsInSR0.first, bitsInSR0.second);
}

// globalDSSID is the combined value of sliceID and dssID on slice.
Value* RTBuilder::getGlobalDSSID()
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

bool RTBuilder::isNonLocalAlloca(uint32_t AddrSpace)
{
    return (AddrSpace != ADDRESS_SPACE_PRIVATE);
}

bool RTBuilder::isNonLocalAlloca(const AllocaInst* AI)
{
    return isNonLocalAlloca(AI->getType()->getPointerAddressSpace());
}

Function* RTBuilder::updateIntrinsicMangle(FunctionType* NewFuncTy, Function& F)
{
    auto* TmpFunc = Function::Create(
        NewFuncTy, F.getLinkage(), F.getName(), F.getParent());
    Optional<Function*> NewFunc =
        Intrinsic::remangleIntrinsicFunction(TmpFunc);
    TmpFunc->eraseFromParent();

    if (NewFunc.hasValue())
    {
        auto* Func = *NewFunc;
        Func->copyMetadata(&F, 0);
        Func->copyAttributesFrom(&F);
        return Func;
    }

    IGC_ASSERT_MESSAGE(0, "remangle failed?");
    return nullptr;
}

Value* RTBuilder::getDispatchRayIndex(
    RTBuilder::SWHotZonePtrVal* SWHotZonePtr, uint32_t Dim)
{
    IGC_ASSERT_MESSAGE((Dim < 3), "Dim out of bounds!");

    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
    {
        Value* Ptr = this->_gepof_dispRaysIndex_v2(
            SWHotZonePtr,
            this->getInt32(Dim),
            VALUE_NAME("&DispatchRayIndex[" + Twine(Dim) + "]"));

        Value* Load = this->CreateLoad(Ptr,
            VALUE_NAME("DispatchRayIndex" + Twine(Dim)));

        return Load;
    }

    // Just load as a dword and extract the upper bits
    static_assert(offsetof(StackPtrAndBudges, StackOffset) == 0, "changed?");
    static_assert(offsetof(StackPtrAndBudges, BudgeBits) == 2, "changed?");

    Value* Ptr = this->_gepof_BudgeBits_v1(SWHotZonePtr, VALUE_NAME("&Budges"));

    Value* Budges = this->CreateLoad(Ptr, VALUE_NAME("Budges"));

    Value* CompressedPtr =
        this->_gepof_CompressedDispatchRayIndices_v1(SWHotZonePtr,
            VALUE_NAME("&CompressedDispatchRayIndices"));

    Value* CompressedVal = this->CreateLoad(
        CompressedPtr, VALUE_NAME("CompressedVal"));

    auto Bits = [&](uint32_t Dim)
    {
        Value* NeededBits = this->CreateLShr(
            Budges, Dim * (uint64_t)StackPtrAndBudges::Bits::DimBits);
        NeededBits = this->CreateAnd(
            NeededBits,
            this->getInt16(BITMASK((uint32_t)StackPtrAndBudges::Bits::DimBits)),
            VALUE_NAME("NeededBits[" + Twine(Dim) + "]"));

        return NeededBits;
    };

    Value* PriorBits = this->getInt16(0);
    for (uint32_t i = 0; i < Dim; i++)
    {
        PriorBits = this->CreateAdd(PriorBits, Bits(i), VALUE_NAME("PriorBits"));
    }

    Value* SelfBits = this->CreateZExt(Bits(Dim), this->getInt32Ty());
    SelfBits = this->CreateShl(this->getInt32(1), SelfBits);
    SelfBits = this->CreateSub(
        SelfBits, this->getInt32(1), VALUE_NAME("SelfBits"));

    Value* DispatchRayIndex = this->CreateLShr(
        CompressedVal, this->CreateZExt(PriorBits, this->getInt32Ty()));
    DispatchRayIndex = this->CreateAnd(
        DispatchRayIndex,
        SelfBits,
        VALUE_NAME("DispatchRayIndex[" + Twine(Dim) + "]"));

    return DispatchRayIndex;
}

void RTBuilder::setDispatchRayIndices(
    RTBuilder::SWHotZonePtrVal* SWHotZonePtr,
    const std::vector<Value*>& Indices)
{
    IGC_ASSERT_MESSAGE((Indices.size() == 3), "Wrong size!");

    if (IGC_IS_FLAG_DISABLED(EnableCompressedRayIndices))
    {
        for (uint32_t Dim = 0; Dim < Indices.size(); Dim++)
        {
            auto* V = Indices[Dim];
            Value* Ptr = this->_gepof_dispRaysIndex_v2(
                SWHotZonePtr,
                this->getInt32(Dim),
                VALUE_NAME("&DispatchRayIndex[" + Twine(Dim) + "]"));

            this->CreateStore(V, Ptr);
        }

        return;
    }

    auto* M = this->GetInsertBlock()->getModule();

    Value* Budges = this->getInt16(0);
    Value* CompressedVal = this->getInt32(0);
    Value* CurLoc = this->getInt32(0);
    auto* Ctlz = Intrinsic::getDeclaration(
        M, Intrinsic::ctlz, this->getInt32Ty());
    for (uint32_t i = 0; i < 3; i++)
    {
        auto& V = Indices[i];
        auto* Lzd = this->CreateCall2(
            Ctlz, V, this->getFalse(), VALUE_NAME("Lzd"));
        auto* UsedBits = this->CreateSub(
            this->getInt32(32),
            Lzd,
            VALUE_NAME("UsedBits[" + Twine(i) + "]"));

        auto* ShiftedV = this->CreateShl(
            V, CurLoc, VALUE_NAME("Shifted[" + Twine(i) + "]"));
        CompressedVal = this->CreateOr(
            CompressedVal, ShiftedV, VALUE_NAME("CompressedVal"));
        Budges = this->CreateOr(
            Budges,
            this->CreateShl(
                this->CreateTrunc(UsedBits, this->getInt16Ty()),
                this->getInt16(i * (uint32_t)StackPtrAndBudges::Bits::DimBits)),
            VALUE_NAME("CurBudges"));

        CurLoc = this->CreateAdd(CurLoc, UsedBits, VALUE_NAME("CurLoc"));
    }

    Value* BudgesPtr = this->_gepof_BudgeBits_v1(SWHotZonePtr,
        VALUE_NAME("&Budges"));

    Value* CompressedPtr =
        this->_gepof_CompressedDispatchRayIndices_v1(SWHotZonePtr,
            VALUE_NAME("&CompressedDispatchRayIndices"));

    this->CreateStore(Budges, BudgesPtr);
    this->CreateStore(CompressedVal, CompressedPtr);
}

void RTBuilder::storeContinuationAddress(
    TraceRayRTArgs &Args,
    Type *PayloadTy,
    ContinuationHLIntrinsic* intrin,
    RTBuilder::SWStackPtrVal* StackFrameVal)
{
    IGC_ASSERT_MESSAGE(intrin->isValidContinuationID(), "invalid ID!");

    Value* contId = nullptr;

    IGC_ASSERT_MESSAGE((this->Ctx.type == ShaderType::RAYTRACING_SHADER), "only raytracing shaders for now!");

    auto& RayCtx = static_cast<const RayDispatchShaderContext&>(this->Ctx);

    if (RayCtx.requiresIndirectContinuationHandling())
    {
        auto* modMD = RayCtx.getModuleMetaData();
        auto& FuncMD = modMD->FuncMD;
        auto I = FuncMD.find(intrin->getContinuationFn());

        std::optional<uint32_t> SlotNum;
        if (I != FuncMD.end())
            SlotNum = I->second.rtInfo.SlotNum;

        Value* ShaderRecordPtr = nullptr;

        if (SlotNum)
        {
            // Compute the KSP pointer by subtracting back from the local
            // pointer
            IGC_ASSERT(ShaderIdentifier::NumSlots > *SlotNum);
            auto* LP = this->getLocalBufferPtr(None);
            const int32_t Offset =
                (ShaderIdentifier::NumSlots - *SlotNum) * sizeof(KSP);
            ShaderRecordPtr = this->CreateGEP(LP, this->getInt32(-Offset));
        }
        else
        {
            Function* pFunc = GenISAIntrinsic::getDeclaration(
                this->GetInsertBlock()->getModule(),
                GenISAIntrinsic::GenISA_GetShaderRecordPtr);
            auto* Cast = this->CreatePointerBitCastOrAddrSpaceCast(
                intrin->getContinuationFn(),
                this->getInt8PtrTy());
            ShaderRecordPtr = this->CreateCall(
                pFunc,
                Cast,
                VALUE_NAME("&ShaderRecord"));
        }
        contId = this->CreatePtrToInt(
            ShaderRecordPtr, this->getInt64Ty());
    }
    else
    {
        contId = this->getInt64(intrin->getContinuationID());
    }

    Value* Ptr = Args.getReturnIPPtr(
        *this, PayloadTy, StackFrameVal, VALUE_NAME("&NextFrame"));

    this->CreateStore(contId, Ptr);
}

void RTBuilder::storePayload(
    TraceRayRTArgs &Args,
    Value* Payload,
    RTBuilder::SWStackPtrVal* StackFrameVal)
{
    auto* Ptr = Args.getPayloadPtr(
        *this, Payload->getType(), StackFrameVal, VALUE_NAME("&NextFrame"));

    this->CreateStore(Payload, Ptr);

    if (Args.needPayloadPadding())
    {
        auto* PadPtr = Args.getPayloadPaddingPtr(
            *this, Payload->getType(), StackFrameVal, VALUE_NAME("&NextFrame"));

        // This is padded out to ensure we don't have a partial write. We just
        // write '0' here by convention but it shouldn't be read anywhere.
        this->CreateStore(this->getInt32(0), PadPtr);
    }
}

// This function loads the ray payload from the RTStack that was previously stored
// by a caller function. This is the way that arguments are passed between functions
// in our raytracing implementation.
Instruction* RTBuilder::LowerPayload(
    Function* F, RTArgs& Args, RTBuilder::SWStackPtrVal* FrameAddr)
{
    Argument* Arg = Args.getPayloadArg(F);
    // not specified
    if (!Arg)
        return nullptr;

    if (Arg->use_empty())
        return nullptr;

    auto* IP = isa<Instruction>(FrameAddr) ?
        cast<Instruction>(FrameAddr)->getNextNode() :
        &*F->getEntryBlock().getFirstInsertionPt();

    IRBuilder<> IRB(IP);

    auto* Ptr = Args.getPayloadPtr(
        IRB,
        Arg->getType(),
        FrameAddr,
        VALUE_NAME("&Frame"));

    auto* stackVal = IRB.CreateLoad(Ptr);

    stackVal->takeName(Arg);

    Arg->replaceAllUsesWith(stackVal);

    return stackVal;
}

void RTBuilder::loadCustomHitAttribsFromStack(
    Function &F,
    RTArgs &Args,
    RTBuilder::SWStackPtrVal *FrameAddr)
{
    Argument* Arg = Args.getHitAttribArg(&F);
    // not specified
    if (!Arg)
        return;

    if (Arg->use_empty())
        return;

    auto* IP = isa<Instruction>(FrameAddr) ?
        cast<Instruction>(FrameAddr)->getNextNode() :
        &*F.getEntryBlock().getFirstInsertionPt();

    RTBuilder IRB(IP, Args.Ctx);

    Type* AttrTy = Arg->getType()->getPointerElementType();
    Value* CustomHitAttrPtr = Args.getCustomHitAttribPtr(IRB, FrameAddr, AttrTy);

    CustomHitAttrPtr->takeName(Arg);
    Arg->replaceAllUsesWith(CustomHitAttrPtr);
}

//Lowering of Intersection Attribute -  lowers Ray Intersection attribute which consists
//of 2 floats (float2 barycentrics). This info is pulled from the MemHit
// members from RTStack.  Custom attributes (from an intersection shader)
// can be of arbitrary type.
//These attributes are only applicable to anyHit and closestHit Shaders
void RTBuilder::lowerIntersectionAttributeFromMemHit(
    Function &F,
    const RTArgs &Args,
    StackPointerVal *StackPointer)
{
    Argument* Arg = Args.getHitAttribArg(&F);
    // not specified
    if (!Arg)
        return;

    if (Arg->use_empty())
        return;

    auto* IP = isa<Instruction>(StackPointer) ?
        cast<Instruction>(StackPointer)->getNextNode() :
        &*F.getEntryBlock().getFirstInsertionPt();

    RTBuilder builder(IP, Args.Ctx);

    Value* uPtr = builder.getHitUPtr(StackPointer, Args.FuncType);

    uint32_t SWStackAddrSpace =
        RTBuilder::getSWStackAddrSpace(*Args.Ctx.getModuleMetaData());

    // write the values to a stack slot
    auto *Storage = builder.CreateAlloca(
        IGCLLVM::FixedVectorType::get(builder.getFloatTy(), 2), nullptr, "", SWStackAddrSpace);

    builder.CreateMemCpy(
        Storage, 4,
        uPtr, 4,
        builder.getInt64(2 * sizeof(float)));

    auto *NewAddr = builder.CreatePointerBitCastOrAddrSpaceCast(
        Storage,
        Arg->getType());

    NewAddr->takeName(Arg);

    Arg->replaceAllUsesWith(NewAddr);
}

void RTBuilder::setRayInfo(
    RTBuilder::StackPointerVal* StackPointer, Value* V, uint32_t Idx, uint32_t BvhLevel)
{
    auto* Ptr = this->getRayInfoPtr(StackPointer, Idx, BvhLevel);
    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getRootNodePtr(Value* BVHPtr)
{
    if (IGC_IS_FLAG_ENABLED(ForceNullBVH))
    {
        // Force all BVHs to be null. Infinitely fast ray traversal.
        return this->getInt64(0);
    }

    if (auto Offset = Ctx.BVHFixedOffset)
    {
        // If the BVH has provided us a fixed offset, instead of doing the below
        // loading of the offset we can just do:
        //
        // char* bvh_root = (bvh) ? bvh + fixed_offset : 0;
        Value* Cond = nullptr;
        if (IGC_IS_FLAG_ENABLED(DisableNullBVHCheck))
        {
            Cond = this->getInt1(true);
        }
        else
        {
            Cond = this->CreateICmpNE(
                BVHPtr, Constant::getNullValue(BVHPtr->getType()));
        }
        Value* Cast = this->CreatePointerCast(
            BVHPtr,
            this->getInt64Ty(),
            VALUE_NAME("&BVH"));
        Value* OffsetPtr = this->CreateAdd(Cast, this->getInt64(*Offset));
        constexpr uint64_t Mask = QWBITMASK((uint32_t)MemRay::Bits::rootNodePtr);
        OffsetPtr = this->CreateAnd(
            OffsetPtr,
            this->getInt64(Mask),
            VALUE_NAME("OffsetPtr"));
        return this->CreateSelect(
            Cond, OffsetPtr, this->getInt64(0), VALUE_NAME("rootNodePtr"));
    }

    auto* M = this->GetInsertBlock()->getModule();
    Value* bitcast = this->CreatePointerCast(
        BVHPtr,
        getBVHPtrTy(*M),
        VALUE_NAME("&BVH"));

    auto* OffsetPtr = this->_gepof_BVH_rootNodeOffset(
        bitcast,
        VALUE_NAME("&rootNodeOffset"));
    static_assert(sizeof(BVH::rootNodeOffset) == 8, "offset size changed?");
    // TODO: offset is 64-bit but we only read the lower 32-bit?
    OffsetPtr = this->CreateBitCast(
        OffsetPtr,
        PointerType::get(
            this->getInt32Ty(), OffsetPtr->getType()->getPointerAddressSpace()),
        VALUE_NAME("&rootNodeOffset_lower"));

    auto fetchRootNodePtr = [&]()
    {
        Value* rootNodeOffset = this->CreateLoad(
            OffsetPtr, VALUE_NAME("rootNodeOffset"));
        Value* bvhPtr = this->CreatePtrToInt(BVHPtr, this->getInt64Ty());
        Value* rootNodePtr = this->CreateAdd(
            bvhPtr, this->CreateZExt(rootNodeOffset, this->getInt64Ty()));
        constexpr uint64_t Mask = QWBITMASK((uint32_t)MemRay::Bits::rootNodePtr);
        rootNodePtr = this->CreateAnd(
            rootNodePtr,
            this->getInt64(Mask),
            VALUE_NAME("rootNodePtr"));

        return rootNodePtr;
    };

    if (IGC_IS_FLAG_ENABLED(DisableNullBVHCheck))
    {
        return fetchRootNodePtr();
    }

    // The DXR spec says:
    // "Specifying a NULL acceleration structure forces a miss."
    //
    // Previously, we unconditionally loaded the offset to the bvh out of the
    // acceleration structure and added it to the base to get the rootNodePtr.
    // We can't do this because the pointer may be null from the application.
    // We will do the below check instead:
    //
    // char* bvh_root = (bvh) ? bvh + bvh->root_node_offset : 0;
    //
    // Note: We'd like to remove this check in the future. The BVH layout may
    // be reorganized such that the root node will be located in a fixed
    // position so we can avoid the load of the offset.
    auto* CurIP = &*this->GetInsertPoint();
    auto* CurBB = this->GetInsertBlock();

    auto* JoinBB = CurBB->splitBasicBlock(
        CurIP,
        VALUE_NAME("LoadBVHOffsetJoin"));
    auto* LoadBVHOffsetBB = BasicBlock::Create(
        M->getContext(),
        VALUE_NAME("LoadBVHOffset"),
        CurBB->getParent(),
        JoinBB);
    CurBB->getTerminator()->eraseFromParent();

    this->SetInsertPoint(CurBB);
    auto* Cond = this->CreateICmpNE(
        OffsetPtr, Constant::getNullValue(OffsetPtr->getType()));
    this->CreateCondBr(Cond, LoadBVHOffsetBB, JoinBB);

    this->SetInsertPoint(LoadBVHOffsetBB);
    Value* rootNodePtr = fetchRootNodePtr();
    this->CreateBr(JoinBB);

    this->SetInsertPoint(&*JoinBB->begin());
    auto* Ptr = this->CreatePHI(
        rootNodePtr->getType(), 2, VALUE_NAME("rootNodePtr"));
    Ptr->addIncoming(rootNodePtr, LoadBVHOffsetBB);
    Ptr->addIncoming(this->getInt64(0), CurBB);

    // reset the IP
    this->SetInsertPoint(CurIP);
    return Ptr;
}

Value* RTBuilder::getNodePtrAndFlagsPtr(
    StackPointerVal* StackPointer,
    uint32_t BvhLevel)
{
    return _gepof_topOfNodePtrAndFlags(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::topOfNodePtrAndFlags"));
}

Value* RTBuilder::getNodePtrAndFlags(
    StackPointerVal* StackPointer,
    uint32_t BvhLevel)
{
    return this->CreateLoad(this->getNodePtrAndFlagsPtr(StackPointer, BvhLevel));
}

void RTBuilder::setNodePtrAndFlags(
    RTBuilder::StackPointerVal* StackPointer, Value* V, uint32_t BvhLevel)
{
    this->CreateStore(V, this->getNodePtrAndFlagsPtr(StackPointer, BvhLevel));
}

void RTBuilder::setHitGroupPtrAndStride(
    RTBuilder::StackPointerVal* StackPointer, Value* V, uint32_t BvhLevel)
{
    auto* Ptr = _gepof_hitGroupShaderRecordInfo(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::hitGroupShaderRecordInfo"));
    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getMissShaderPtr(
    RTBuilder::StackPointerVal* StackPointer, uint32_t BvhLevel)
{
    auto* Ptr = _gepof_missShaderRecordInfo(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::missShaderRecordInfo"));
    return this->CreateLoad(Ptr);
}

void RTBuilder::setMissShaderPtr(
    RTBuilder::StackPointerVal* StackPointer, Value* V, uint32_t BvhLevel)
{
    auto* Ptr = _gepof_missShaderRecordInfo(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::missShaderRecordInfo"));
    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getInstLeafPtrAndRayMask(
    RTBuilder::StackPointerVal* StackPointer, uint32_t BvhLevel)
{
    auto* Ptr = this->_gepof_topOfInstanceLeafPtr(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::topOfInstanceLeafPtr"));
    return this->CreateLoad(Ptr);
}

void RTBuilder::setInstLeafPtrAndRayMask(
    RTBuilder::StackPointerVal* StackPointer, Value* V, uint32_t BvhLevel)
{
    auto* Ptr = this->_gepof_topOfInstanceLeafPtr(
        StackPointer, this->getInt32(BvhLevel),
        VALUE_NAME("&MemRay::topOfInstanceLeafPtr"));
    this->CreateStore(V, Ptr);
}

void RTBuilder::setCommittedHitT(
    RTBuilder::StackPointerVal* StackPointer, Value* V)
{
    auto* Ptr = this->_gepof_CommittedHitT(StackPointer,
        VALUE_NAME("&committedHit.t"));

    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getPotentialHitT(RTBuilder::StackPointerVal* StackPointer)
{
    auto* Ptr = this->_gepof_PotentialHitT(StackPointer);

    Value* RayTFar = this->CreateLoad(
        Ptr,
        VALUE_NAME("potentialHit.t"));
    return RayTFar;
}

void RTBuilder::setPotentialHitT(RTBuilder::StackPointerVal* StackPointer, Value* V)
{
    auto* Ptr = this->_gepof_PotentialHitT(StackPointer,
        VALUE_NAME("&potentialHit.t"));

    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getPotentialHitTopPrimLeafPtr(RTBuilder::StackPointerVal* StackPointer)
{
    auto* Ptr = this->_gepof_PotentialHitTopOfPrimLeafPtr(StackPointer,
        VALUE_NAME("&potentialHit.topOfPrimLeafPtr"));

    // upper 22 bits contains hitGroupRecPtr0
    Value* PotentialPrimVal = this->CreateLoad(
        Ptr,
        VALUE_NAME("potential_prim_leaf"));

    return PotentialPrimVal;
}

void RTBuilder::setCommittedHitTopPrimLeafPtr(RTBuilder::StackPointerVal* StackPointer, Value *V)
{
    auto* Ptr = this->_gepof_CommittedHitTopOfPrimLeafPtr(StackPointer,
        VALUE_NAME("&committedHit.topOfPrimLeafPtr"));
    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getPotentialHitTopInstLeafPtr(RTBuilder::StackPointerVal* StackPointer)
{
    auto* Ptr = this->_gepof_PotentialHitTopOfInstLeafPtr(StackPointer,
        VALUE_NAME("&potentialHit.topOfInstLeafPtr"));

    // upper 22 bits contains hitGroupRecPtr1
    Value *PotentialInstVal = this->CreateLoad(
        Ptr,
        VALUE_NAME("potential_inst_leaf"));

    return PotentialInstVal;
}

void RTBuilder::setCommittedHitTopInstLeafPtr(RTBuilder::StackPointerVal* StackPointer, Value* V)
{
    auto* Ptr = this->_gepof_CommittedHitTopOfInstLeafPtr(StackPointer,
        VALUE_NAME("&committedHit.topOfInstLeafPtr"));
    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getHitTopOfPrimLeafPtr(
    StackPointerVal* perLaneStackPtr,
    IGC::CallableShaderTypeMD ShaderTy)
{
    Value* topOfPrimLeafPtr = nullptr;
    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        topOfPrimLeafPtr = this->_gepof_CommittedHitTopOfPrimLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&committedHit.topOfPrimLeafPtr"));
    }
    else
    {
        topOfPrimLeafPtr = this->_gepof_PotentialHitTopOfPrimLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&potentialHit.topOfPrimLeafPtr"));
    }

    Value* loadValue = this->CreateLoad(topOfPrimLeafPtr);
    return loadValue;
}

//Note: this is NOT to return MemHit::topOfInstLeafPtr (the whole QW)
//instead, it's to return BVH::instLeaf[i]
Value* RTBuilder::getHitInstanceLeafPtr(
    StackPointerVal* perLaneStackPtr,
    IGC::CallableShaderTypeMD ShaderTy)
{
    Value* topOfInstanceLeafPtr = nullptr;

    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        topOfInstanceLeafPtr = this->_gepof_CommittedHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&committedHit.topOfInstLeafPtr"));
    }
    else
    {
        topOfInstanceLeafPtr = this->_gepof_PotentialHitTopOfInstLeafPtr(
            perLaneStackPtr,
            VALUE_NAME("&potentialHit.topOfInstLeafPtr"));
    }

    //Read Instance Leaf Ptr
    Value* ptrValue = this->getInstanceLeafPtr(topOfInstanceLeafPtr);
    return ptrValue;
}

Value* RTBuilder::isDoneBitSet(Value* HitInfoVal)
{
    Value* rayQueryDone = this->CreateAnd(
        HitInfoVal,
        this->getInt32(BIT((uint32_t)MemHit::Offset::done)));

    return this->CreateICmpNE(
        rayQueryDone,
        this->getInt32(0),
        VALUE_NAME("is_done"));
}

Value* RTBuilder::isDoneBitNotSet(Value* HitInfoVal)
{
    return this->CreateNot(this->isDoneBitSet(HitInfoVal), VALUE_NAME("!done"));
}

Value* RTBuilder::getMemHitBvhLevel(Value* MemHitInfoVal)
{
    auto *ShiftVal = this->CreateLShr(
        MemHitInfoVal, (uint32_t)MemHit::Offset::bvhLevel);
    constexpr uint32_t Mask = BITMASK((uint32_t)MemHit::Bits::bvhLevel);
    auto* Level = this->CreateAnd(ShiftVal, Mask, VALUE_NAME("bvhLevel"));
    return Level;
}

Value* RTBuilder::getPotentialHitBvhLevel(RTBuilder::StackPointerVal* StackPointer)
{
    auto *DW = this->getHitInfoDWord(StackPointer, CallableShaderTypeMD::AnyHit, VALUE_NAME("BvhLevelDW"));
    return this->getMemHitBvhLevel(DW);
}

Value* RTBuilder::getPotentialHitInfo(RTBuilder::StackPointerVal* StackPointer, const Twine& Name)
{
    return this->getHitInfoDWord(StackPointer, CallableShaderTypeMD::AnyHit, Name);
}

Value* RTBuilder::getHitValid(RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* validDW = this->getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("ValidDW"));
    Value* isValidBit = CreateAnd(
        validDW,
        getInt32(BIT((uint32_t)MemHit::Offset::valid)));
    return CreateICmpNE(
        isValidBit, getInt32(0), VALUE_NAME("valid"));
}

//MemHit.valid = true;
void RTBuilder::setHitValid(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* ValidDW = getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("valid"));
    ValidDW = CreateOr(
        ValidDW,
        getInt32(1U << (uint32_t)MemHit::Offset::valid));
    setHitInfoDWord(StackPointer, ShaderTy, ValidDW, VALUE_NAME("valid"));
}

Value* RTBuilder::getSyncTraceRayControl(Value* ptrCtrl)
{
    return this->CreateLoad(ptrCtrl, VALUE_NAME("rayQueryObject.traceRayControl"));
}

void RTBuilder::setSyncTraceRayControl(Value* ptrCtrl, unsigned ctrl)
{
    Type* eleType = cast<PointerType>(ptrCtrl->getType())->getElementType();
    this->CreateStore(llvm::ConstantInt::get(eleType, ctrl), ptrCtrl);
}

Value* RTBuilder::getHitUPtr(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* Ptr = nullptr;
    if (ShaderTy == CallableShaderTypeMD::ClosestHit)
    {
        Ptr = this->_gepof_CommittedHitU(StackPointer,
            VALUE_NAME("&committedHit.u"));
    }
    else if (ShaderTy == CallableShaderTypeMD::AnyHit)
    {
        Ptr = this->_gepof_PotentialHitU(StackPointer,
            VALUE_NAME("&potentialHit.u"));
    }
    else
    {
        IGC_ASSERT_MESSAGE(0, "Intersection attributes only apply to ClosestHit and AnyHit shaders");
    }

    return Ptr;
}

Value* RTBuilder::getHitBaryCentricPtr(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim)
{
    auto* Ptr = this->getHitUPtr(StackPointer, ShaderTy);
    IGC_ASSERT_MESSAGE(dim == 0 || dim == 1, "Only U V are supported.");
    if (dim == 1)
    {
        IGC_ASSERT(llvm::isa<llvm::GetElementPtrInst>(Ptr));
        if (auto* GEP = dyn_cast<llvm::GetElementPtrInst>(Ptr))
        {
            static_assert(offsetof(MemHit, v) - offsetof(MemHit, u) == sizeof(float));
            llvm::Value* idx_dim0 = GEP->getOperand(GEP->getNumOperands() - 1);
            llvm::Value* idx_dim1 = this->CreateAdd(idx_dim0, this->getInt32(1));
            GEP->setOperand(GEP->getNumOperands() - 1, idx_dim1);
        }
    }

    return Ptr;
}

Value* RTBuilder::getHitBaryCentric(
    RTBuilder::StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim)
{
    return this->CreateLoad(getHitBaryCentricPtr(StackPointer, ShaderTy, dim), (dim ? VALUE_NAME("MemHit.v") : VALUE_NAME("MemHit.u")));
}

Value* RTBuilder::setHitBaryCentric(
    RTBuilder::StackPointerVal* StackPointer, Value* V, IGC::CallableShaderTypeMD ShaderTy, uint32_t dim)
{
    return this->CreateStore(V, getHitBaryCentricPtr(StackPointer, ShaderTy, dim));
}

Value* RTBuilder::getInstContToHitGroupIndex(RTBuilder::StackPointerVal* perLaneStackPtr,
    IGC::CallableShaderTypeMD ShaderTy)
{
    auto* instLeafPtr = this->getHitInstanceLeafPtr(perLaneStackPtr, ShaderTy);
    auto* Ptr = this->_gepof_InstanceLeaf_instContToHitGroupIndex(instLeafPtr,
        VALUE_NAME("&InstanceLeaf...instContToHitGroupIndex"));

    Value* value = this->CreateLoad(Ptr, VALUE_NAME("InstanceLeaf...instContToHitGroupIndex"));
    //Only lower 24bits for the Dword read are valid for Index
    return this->CreateAnd(value, this->getInt32(BITMASK((uint32_t)InstanceLeaf::Part0::Bits::instContToHitGrpIndex)));
}

void RTBuilder::setProceduralHitKind(
    RTArgs &Args,
    RTBuilder::SWStackPtrVal* FrameAddr, Value* V)
{
    // This function is only supposed to be invoked from an intersection shader.
    IGC_ASSERT_MESSAGE(V->getType() == this->getInt32Ty(), "wrong type?");

    auto* Ptr = Args.getHitKindPtr(*this, FrameAddr);

    this->CreateStore(V, Ptr);
}

Value* RTBuilder::getLeafType(
    StackPointerVal* StackPointer, CallableShaderTypeMD ShaderTy)
{
    Value* Val = this->getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("leafTypeDW"));
    Val = this->CreateLShr(
        Val, this->getInt32((uint32_t)MemHit::Offset::leafType));

    Val = this->CreateAnd(
        Val,
        this->getInt32(BITMASK((uint32_t)MemHit::Bits::leafType)),
        VALUE_NAME("leafType"));

    return Val;
}

Value* RTBuilder::getProceduralHitKind(
    RTArgs &Args,
    SWStackPtrVal* FrameAddr)
{
    auto* Ptr = Args.getHitKindPtr(*this, FrameAddr);

    return this->CreateLoad(Ptr, VALUE_NAME("HitKind"));
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

    constexpr uint32_t VA_MSB = 47;
    constexpr uint32_t MSB = 63;
    constexpr uint32_t ShiftAmt = MSB - VA_MSB;
    auto *P2I = this->CreateBitOrPointerCast(Ptr, this->getInt64Ty());
    auto* Canonized = this->CreateShl(P2I, ShiftAmt);
    Canonized = this->CreateAShr(Canonized, ShiftAmt);
    Canonized = this->CreateBitOrPointerCast(Canonized, Ptr->getType(),
        VALUE_NAME(Ptr->getName() + Twine("-canonized")));

    return Canonized;
}

DenseMap<const AllocaInst*, uint32_t>
RTBuilder::getAllocaNumberMap(const Function& F)
{
    DenseMap<const AllocaInst*, uint32_t> M;

    uint32_t Cnt = 0;
    for (auto& I : F.getEntryBlock())
    {
        if (auto * AI = dyn_cast<AllocaInst>(&I))
        {
            if (!RTBuilder::isNonLocalAlloca(AI))
                continue;

            M[AI] = Cnt++;
        }
    }

    return M;
}

DenseMap<uint32_t, const AllocaInst*>
RTBuilder::getNumberAllocaMap(const Function& F)
{
    DenseMap<uint32_t, const AllocaInst*> M;

    uint32_t Cnt = 0;
    for (auto& I : F.getEntryBlock())
    {
        if (auto * AI = dyn_cast<AllocaInst>(&I))
        {
            if (!RTBuilder::isNonLocalAlloca(AI))
                continue;

            M[Cnt++] = AI;
        }
    }

    return M;
}

Value* RTBuilder::createAllocaNumber(const AllocaInst* AI, uint32_t Number)
{
    auto* M = this->GetInsertBlock()->getModule();
    Function* Func = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_AllocaNumber,
        AI->getType());

    Value* Call = this->CreateCall(
        Func,
        this->getInt32(Number), VALUE_NAME("AllocaNumber-" + Twine(Number)));

    return Call;
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

void RTBuilder::setNoAlias(CallInst* CI)
{
    auto Attrs = CI->getAttributes();
    IGCLLVM::AttrBuilder AB{ CI->getContext(), Attrs.getAttributes(AttributeList::ReturnIndex) };
    AB.addAttribute(Attribute::AttrKind::NoAlias);
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

Value* RTBuilder::getLocalBufferPtr(llvm::Optional<uint32_t> root_sig_size)
{
    auto* M = this->GetInsertBlock()->getModule();

    Function* Func = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_LocalBufferPointer,
        this->getInt8PtrTy(ADDRESS_SPACE_CONSTANT));

    constexpr uint32_t LocalAlign = offsetof(ShaderRecord, LocalRootSig);

    static_assert(alignof(ShaderRecord) == 32, "changed?");
    static_assert(LocalAlign == 32, "changed?");

    CallInst *CI = this->CreateCall(Func, None, VALUE_NAME("localPtr"));

    /* Add the dereferenceable(root_sig_size) attribute to the return pointer */
    if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
    {
        if (root_sig_size)
        {
            this->setDereferenceable(CI, *root_sig_size);
        }
        this->setNoAlias(CI);
    }

    this->setReturnAlignment(CI, LocalAlign);

    return CI;
}

CallInst* RTBuilder::getInlineData(Type* RetTy, uint32_t QwordOffset, uint32_t Alignment, llvm::Optional<uint32_t> root_sig_size /* = None */)
{
    uint32_t AddrSpace = RetTy->getPointerAddressSpace();

    auto* M = this->GetInsertBlock()->getModule();
    Function* pFunc = GenISAIntrinsic::getDeclaration(
        M,
        GenISAIntrinsic::GenISA_InlinedData,
        this->getInt8PtrTy(AddrSpace)
    );

    CallInst* CI = this->CreateCall(
        pFunc,
        this->getInt32(QwordOffset),
        VALUE_NAME("inlineData")
    );

    if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        this->setReturnAlignment(CI, Alignment);

    /* Add the dereferenceable(root_sig_size) attribute to the return pointer */
    // If the optional is None, I wouldn't even set the dereferenceable attribute, rather than default setting it to zero.
    if (root_sig_size) {
        if (IGC_IS_FLAG_DISABLED(DisableRaytracingIntrinsicAttributes))
        {
            this->setDereferenceable(CI, *root_sig_size);
        }
    }

    return CI;
}

PayloadPtrIntrinsic* RTBuilder::getPayloadPtrIntrinsic(Value* PayloadPtr)
{
    Module* M = this->GetInsertBlock()->getModule();
    auto *CI = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M,
            GenISAIntrinsic::GenISA_PayloadPtr,
            PayloadPtr->getType()),
        PayloadPtr,
        VALUE_NAME("&Payload"));
    return cast<PayloadPtrIntrinsic>(CI);
}

ContinuationSignpostIntrinsic* RTBuilder::getContinuationSignpost(
    Value* FrameAddr, Value* Offset)
{
    Module* M = this->GetInsertBlock()->getModule();
    auto *CI = this->CreateCall2(
        GenISAIntrinsic::getDeclaration(
            M,
            GenISAIntrinsic::GenISA_ContinuationSignpost,
            FrameAddr->getType()),
        FrameAddr,
        Offset,
        VALUE_NAME("FrameAddr"));
    return cast<ContinuationSignpostIntrinsic>(CI);
}

Value* RTBuilder::getAsyncStackID()
{
    Module* module = this->GetInsertBlock()->getModule();
    return this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            module, GenISAIntrinsic::GenISA_AsyncStackID),
        None,
        VALUE_NAME("AsyncStackID"));
}

Value* RTBuilder::getSyncStackID()
{
    Module* module = this->GetInsertBlock()->getModule();
    Value* stackID = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            module, GenISAIntrinsic::GenISA_SyncStackID),
        None,
        VALUE_NAME("SyncStackID"));

    return stackID;
}

void RTBuilder::injectPadding(
    Module& M,
    SmallVector<Type*, 4>& Tys,
    uint32_t Align,
    bool IsPacked)
{
    auto& C = M.getContext();
    auto& DL = M.getDataLayout();
    auto* Tmp = StructType::get(C, Tys, IsPacked);
    auto Layout = DL.getStructLayout(Tmp);
    uint64_t Size = Layout->getSizeInBytes();
    uint64_t Diff = iSTD::Align(Size, Align) - Size;
    if (Diff != 0)
    {
        auto* VTy = ArrayType::get(Type::getInt8Ty(C), Diff);
        Tys.push_back(VTy);
    }
}

bool RTBuilder::checkAlign(
    Module &M,
    StructType* StructTy,
    uint32_t Align)
{
    auto& DL = M.getDataLayout();
    auto Layout = DL.getStructLayout(StructTy);

    uint64_t Size = Layout->getSizeInBytes();
    uint64_t Diff = Size % Align;
    return (Diff == 0);
}

uint32_t RTBuilder::getRTAsyncStackStatefulAddrSpace(const ModuleMetaData &MMD)
{
    auto& rtInfo = MMD.rtInfo;
    IGC_ASSERT_MESSAGE(rtInfo.RTAsyncStackAddrspace != UINT_MAX, "not initialized?");
    return rtInfo.RTAsyncStackAddrspace;
}

uint32_t RTBuilder::getSWHotZoneStatefulAddrSpace(const ModuleMetaData &MMD)
{
    auto& rtInfo = MMD.rtInfo;
    IGC_ASSERT_MESSAGE(rtInfo.SWHotZoneAddrspace != UINT_MAX, "not initialized?");
    return rtInfo.SWHotZoneAddrspace;
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

uint32_t RTBuilder::getSWStackAddrSpace(const ModuleMetaData &MMD)
{
    uint32_t SWStackAddrSpace = ADDRESS_SPACE_GLOBAL;

    if (MMD.rtInfo.SWStackSurfaceStateOffset)
        SWStackAddrSpace = getSWStackStatefulAddrSpace(MMD);

    return SWStackAddrSpace;
}

static const char* RaytracingTypesMDName = "igc.magic.raytracing.types";

// If you need to add another type to the metadata, add a slot for it here
// and add create a getter function below to retrieve it.
enum class RaytracingType
{
    RTStack2Stateless     = 0,
    RTStack2Stateful      = 1,
    RayDispatchGlobalData = 2,
    InstanceLeaf          = 3,
    QuadLeaf              = 4,
    ProceduralLeaf        = 5,
    BVH                   = 6,
    HWRayData2            = 7,
    SWHotZone             = 8,
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

Type* RTBuilder::getRTStack2PtrTy(
    const CodeGenContext &Ctx, RTBuilder::RTMemoryAccessMode Mode, bool async)
{
    IGC_ASSERT_MESSAGE((Mode == RTBuilder::STATELESS || Mode == RTBuilder::STATEFUL), "unknown?");

    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        uint32_t AddrSpace = (Mode == RTBuilder::STATELESS) ?
            ADDRESS_SPACE_GLOBAL :
            (async ? getRTAsyncStackStatefulAddrSpace(*Ctx.getModuleMetaData()) : getRTSyncStackStatefulAddrSpace(*Ctx.getModuleMetaData()));
        auto* Ty = _gettype_RTStack2(*Ctx.getModule());
        return setRTTypeMD(
            *Ctx.getModule(),
            Idx,
            TypesMD,
            Ty,
            RTStackFormat::getRTStackHeaderSize(MAX_BVH_LEVELS),
            AddrSpace);
    };

    auto RTType = (Mode == RTBuilder::STATELESS) ?
        RaytracingType::RTStack2Stateless : RaytracingType::RTStack2Stateful;

    return lazyGetRTType(*Ctx.getModule(), RTType, addTy);
}

Type* RTBuilder::getRayDispatchGlobalDataPtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_RayDispatchGlobalData(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(RayDispatchGlobalData),
            ADDRESS_SPACE_GLOBAL);
    };

    return lazyGetRTType(M, RaytracingType::RayDispatchGlobalData, addTy);
}

Type* RTBuilder::getHWRayData2PtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_HWRayData2(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(HWRayData2),
            ADDRESS_SPACE_GLOBAL);
    };

    return lazyGetRTType(M, RaytracingType::HWRayData2, addTy);
}

Type* RTBuilder::getSWHotZonePtrTy(
    const IGC::CodeGenContext& Ctx,
    RTMemoryAccessMode Mode)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto& M = *Ctx.getModule();
        uint32_t AddrSpace = (Mode == RTBuilder::STATELESS) ?
            ADDRESS_SPACE_GLOBAL :
            getSWHotZoneStatefulAddrSpace(*Ctx.getModuleMetaData());
        auto* Ty = IGC_IS_FLAG_ENABLED(EnableCompressedRayIndices) ?
            _gettype_SWHotZone_v1(M) :
            _gettype_SWHotZone_v2(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            getSWHotZoneSize(),
            AddrSpace);
    };

    auto& M = *Ctx.getModule();
    return lazyGetRTType(M, RaytracingType::SWHotZone, addTy);
}

Type* RTBuilder::getInstanceLeafPtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_InstanceLeaf(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(InstanceLeaf),
            ADDRESS_SPACE_GLOBAL);
    };

    return lazyGetRTType(M, RaytracingType::InstanceLeaf, addTy);
}

Type* RTBuilder::getQuadLeafPtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_QuadLeaf(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(QuadLeaf),
            ADDRESS_SPACE_GLOBAL);
    };

    return lazyGetRTType(M, RaytracingType::QuadLeaf, addTy);
}

Type* RTBuilder::getProceduralLeafPtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_ProceduralLeaf(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(ProceduralLeaf),
            ADDRESS_SPACE_GLOBAL);
    };

    return lazyGetRTType(M, RaytracingType::ProceduralLeaf, addTy);
}

Type* RTBuilder::getBVHPtrTy(Module &M)
{
    auto addTy = [&](NamedMDNode *TypesMD, RaytracingType Idx) {
        auto* Ty = _gettype_BVH(M);
        return setRTTypeMD(
            M,
            Idx,
            TypesMD,
            Ty,
            sizeof(BVH),
            ADDRESS_SPACE_CONSTANT);
    };

    return lazyGetRTType(M, RaytracingType::BVH, addTy);
}

// We don't store this in metadata since we generate a new one of these for
// every shader
StructType* RTBuilder::getRTGlobalsAndRootSig(
    Module& M, Type* TypeHoleGlobalRootSig, StringRef Name)
{
    auto *Ty = cast<StructType>(_gettype_RTGlobalsAndRootSig(M, TypeHoleGlobalRootSig));
    Ty->setName(Name);
    return Ty;
}

StructType* RTBuilder::getShaderRecordTy(
    Module& M, Type* TypeHoleLocalRootSig, StringRef Name)
{
    auto *Ty = cast<StructType>(_gettype_ShaderRecord(M, TypeHoleLocalRootSig));
    Ty->setName(Name);
    return Ty;
}

Type* RTBuilder::getInt64PtrTy(unsigned int AddrSpace) const
{
    return Type::getInt64PtrTy(this->Context, AddrSpace);
}

Type* RTBuilder::getInt32PtrTy(unsigned int AddrSpace) const
{
    return Type::getInt32PtrTy(this->Context, AddrSpace);
}

// Find the Insert point which is placed
// after all Allocas and after all the Instructions
// from the optional vector additionalInstructionsToSkip.
Instruction* RTBuilder::getEntryFirstInsertionPt(
    Function &F,
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

const IGC::RayDispatchShaderContext& RTBuilder::RtCtx() const
{
    IGC_ASSERT_MESSAGE(Ctx.type == ShaderType::RAYTRACING_SHADER,
        "only raytracing shaders for now!");
    return static_cast<const RayDispatchShaderContext&>(Ctx);
}

void RTBuilder::printf(Constant* formatStrPtr, ArrayRef<Value*> Args)
{
    auto* M = Ctx.getModule();

    Function* funcPrintf = M->getFunction(PrintfFuncName);

    if (!funcPrintf)
    {
        FunctionType* funcTypePrintf = FunctionType::get(
            getInt32Ty(),
            formatStrPtr->getType(),
            true);
        funcPrintf = Function::Create(funcTypePrintf,
            GlobalValue::ExternalLinkage,
            PrintfFuncName,
            M);
    }

    CreateCall(funcPrintf, Args, VALUE_NAME("callPrintf"));
}

void RTBuilder::printTraceRay(const TraceRayAsyncHLIntrinsic* trace)
{
    auto& LogMgr = RtCtx().LogMgr;
    if (!LogMgr.isEnabled())
        return;

    Constant* printfStr = LogMgr.getFormatString(
        RTLoggingManager::TRACE_RAY,
        [&](StringRef FormatString) {
            return CreateGlobalStringPtr(
                       FormatString,
                       VALUE_NAME("printf.format.RayDesc"));
        });

    Value* argsPrintf[] = {
        printfStr,
        trace->getRayOrig(0),
        trace->getRayOrig(1),
        trace->getRayOrig(2),
        trace->getRayDir(0),
        trace->getRayDir(1),
        trace->getRayDir(2),
        trace->getTMin(),
        trace->getTMax(),
    };

    printf(printfStr, argsPrintf);
}


void RTBuilder::printDispatchRayIndex(const std::vector<Value*>& Indices)
{
    auto& LogMgr = RtCtx().LogMgr;
    if (!LogMgr.isEnabled())
        return;

    Constant* printfStr = LogMgr.getFormatString(
        RTLoggingManager::DISPATCH_RAY_INDEX,
        [&](StringRef FormatString) {
            return CreateGlobalStringPtr(
                       FormatString,
                       VALUE_NAME("printf.format.DispatchRayIndex"));
        });

    SmallVector<Value*, 4> argsPrintf{ printfStr };
    for (auto *Index : Indices)
        argsPrintf.push_back(Index);

    printf(printfStr, argsPrintf);
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

GenIntrinsicInst* RTBuilder::getSpillAnchor(Value* V)
{
    auto* M = GetInsertBlock()->getModule();
    CallInst* Anchor = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            M, GenISAIntrinsic::GenISA_rt_spill_anchor, V->getType()),
        V,
        VALUE_NAME(V->getName() + Twine(".anchor")));
    return cast<GenIntrinsicInst>(Anchor);
}

void RTBuilder::setSpillSize(ContinuationHLIntrinsic& CI, uint32_t SpillSize)
{
    auto& C = CI.getContext();
    MDNode* node = MDNode::get(
        C,
        ConstantAsMetadata::get(
            ConstantInt::get(Type::getInt32Ty(C), SpillSize)));
    CI.setMetadata(RTBuilder::SpillSize, node);
}

Optional<uint32_t> RTBuilder::getSpillSize(const ContinuationHLIntrinsic& CI)
{
    auto* MD = CI.getMetadata(RTBuilder::SpillSize);
    if (!MD)
        return None;

    auto* CMD = cast<ConstantAsMetadata>(MD->getOperand(0));
    auto* C = cast<ConstantInt>(CMD->getValue());
    return static_cast<uint32_t>(C->getZExtValue());
}
