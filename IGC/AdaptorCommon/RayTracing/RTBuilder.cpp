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
    return _get_rtMemBasePtr_Xe();
}


Value* RTBuilder::getMaxBVHLevels(void)
{
    if (!DisableRTGlobalsKnownValues)
        return this->getInt32(MAX_BVH_LEVELS);

    return _get_maxBVHLevels_Xe();
}


Value* RTBuilder::getStatelessScratchPtr(void)
{
    return _get_statelessScratchPtr_Xe();
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
        this->getInt32(1U << (uint32_t)MemHit::RT::Xe::Offset::frontFace),
        VALUE_NAME("isolate_front_face_bit"));
    return this->CreateICmpNE(
        Val,
        this->getInt32(0),
        VALUE_NAME("is_front_face"));
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
        Value* memBasePtr = nullptr;
        {
            memBasePtr = this->getRtMemBasePtr();;
        }
        IGC_ASSERT(memBasePtr != nullptr);

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
            getInt32(BIT((uint32_t)MemHit::RT::Xe::Offset::valid)));
        valid = CreateICmpNE(valid, this->getInt32(0), VALUE_NAME("valid"));
    }
    else
    {
        Value* leafType = this->CreateLShr(
            hitInfo, this->getInt32((uint32_t)MemHit::RT::Xe::Offset::leafType));

        leafType = this->CreateAnd(
            leafType,
            this->getInt32(BITMASK((uint32_t)MemHit::RT::Xe::Bits::leafType)),
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


void RTBuilder::FillRayQueryShadowMemory(Value* shMem, Value* src, uint64_t size, unsigned align)
{
    this->CreateMemCpy(
        this->CreatePointerCast(shMem, PointerType::get(this->getInt32Ty(), ADDRESS_SPACE_PRIVATE)),
        src,
        size,
        align);
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
        doneDW, this->getInt32(BIT((uint32_t)MemHit::RT::Xe::Offset::done)));
    this->setHitInfoDWord(StackPointer, CallableShaderTypeMD::AnyHit, newHitInfo, VALUE_NAME("DoneDW"));
}

void RTBuilder::CreateAbort(RTBuilder::StackPointerVal* StackPointer)
{
    setPotentialDoneBit(StackPointer);
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
    static_assert((uint32_t)MemRay::RT::Xe::Bits::rootNodePtr == 48, "Changed?");
    static_assert((uint32_t)MemRay::RT::Xe::Bits::rayFlags == 16, "Changed?");
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
        this->getInt64(QWBITMASK((uint32_t)MemHit::RT::Xe::Bits::primLeafPtr)),
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

    // We are interested in only the LSB of leafType
    // because we only check if type is procedural.

    static_assert(
        ((NODE_TYPE_PROCEDURAL & 1) == 1) &&
        ((NODE_TYPE_QUAD & 1) == 0) &&
        ((NODE_TYPE_MESHLET & 1) == 0),
        "optimized CommittedStatus broken");

    // At this point valid bit is expected to be right-shifted to the
    // 0th bit.
    Value* leafType = this->CreateAnd(infoKind, this->getInt32(1));
    Value* leafTyCmp = this->CreateICmpEQ(leafType, this->getInt32(NODE_TYPE_PROCEDURAL & 1));

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
            this->getInt32(BITMASK((uint32_t)MemHit::RT::Xe::Bits::primIndexDelta)),
            VALUE_NAME("primLeafIndexDelta"));
        quadPrimIndex = this->CreateAdd(primLeafIndexDelta, primIndex0, VALUE_NAME("primIndex"));
        this->CreateBr(endBlock);
    }

    //ProdecuralLeaf: leafTyCmp == true
    Value* proceduralPrimIndex = nullptr;
    {
        this->SetInsertPoint(prodeduralLeafBB);
        Value* primLeafIndex = this->CreateLShr(primLeafIndexTop,
            this->getInt32((uint32_t)MemHit::RT::Xe::Offset::primLeafIndex));
        primLeafIndex = this->CreateAnd(primLeafIndex,
            this->getInt32(BITMASK((uint32_t)MemHit::RT::Xe::Bits::primLeafIndex)),
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
        this->getInt64(QWBITMASK((uint32_t)MemHit::RT::Xe::Bits::primLeafPtr)),
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

    // We are interested in only the LSB of leafType
    // because we only check if type is procedural.

    static_assert(
        ((NODE_TYPE_PROCEDURAL & 1) == 1) &&
        ((NODE_TYPE_QUAD & 1) == 0) &&
        ((NODE_TYPE_MESHLET & 1) == 0),
        "optimized CommittedStatus broken");

    // At this point valid bit is expected to be right-shifted to the
    // 0th bit.
    Value* leafType = this->CreateAnd(infoKind, this->getInt32(1));
    Value* leafTyCmp = this->CreateICmpEQ(leafType, this->getInt32(NODE_TYPE_PROCEDURAL & 1));

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
        this->getInt32(BITMASK((uint32_t)PrimLeafDesc::RT::Xe::Bits::geomIndex)),
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
    static_assert(offsetof(MemHit::RT::Xe, t) == 0);
    return this->_gepof_CommittedHitT(perLaneStackPtr,
        VALUE_NAME("&CommittedHit"));
}

Value* RTBuilder::getPotentialHitPtr(RTBuilder::StackPointerVal* perLaneStackPtr)
{
    static_assert(offsetof(MemHit::RT::Xe, t) == 0);
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
        this->getInt64(QWBITMASK((uint32_t)MemHit::RT::Xe::Bits::instLeafPtr)),
        VALUE_NAME("42-bit InstanceLeafPtr"));
    Value* ptrValue = this->CreateMul(loadValue, this->getInt64(LeafSize),
        VALUE_NAME("fullInstanceLeafPtr"));
    ptrValue = canonizePointer(ptrValue);
    ptrValue = this->CreateIntToPtr(ptrValue, this->getInstanceLeafPtrTy(M),
        VALUE_NAME("InstanceLeafPtr"));

    return ptrValue;
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
        constexpr uint64_t Mask = QWBITMASK((uint32_t)MemRay::RT::Xe::Bits::rootNodePtr);
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
        constexpr uint64_t Mask = QWBITMASK((uint32_t)MemRay::RT::Xe::Bits::rootNodePtr);
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
        this->getInt32(BIT((uint32_t)MemHit::RT::Xe::Offset::done)));

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
        MemHitInfoVal, (uint32_t)MemHit::RT::Xe::Offset::bvhLevel);
    constexpr uint32_t Mask = BITMASK((uint32_t)MemHit::RT::Xe::Bits::bvhLevel);
    auto* Level = this->CreateAnd(ShiftVal, Mask, VALUE_NAME("bvhLevel"));
    return Level;
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
        getInt32(BIT((uint32_t)MemHit::RT::Xe::Offset::valid)));
    return CreateICmpNE(
        isValidBit, getInt32(0), VALUE_NAME("valid"));
}

//MemHit.valid = true;
void RTBuilder::setHitValid(StackPointerVal* StackPointer, IGC::CallableShaderTypeMD ShaderTy)
{
    Value* ValidDW = getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("valid"));
    ValidDW = CreateOr(
        ValidDW,
        getInt32(1U << (uint32_t)MemHit::RT::Xe::Offset::valid));
    setHitInfoDWord(StackPointer, ShaderTy, ValidDW, VALUE_NAME("valid"));
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
            static_assert(offsetof(MemHit::RT::Xe, v) - offsetof(MemHit::RT::Xe, u) == sizeof(float));
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
    return this->CreateLoad(getHitBaryCentricPtr(StackPointer, ShaderTy, dim),
        dim ? VALUE_NAME("MemHit.v") : VALUE_NAME("MemHit.u"));
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
    return this->CreateAnd(value, this->getInt32(BITMASK((uint32_t)InstanceLeaf::Part0::RT::Xe::Bits::instContToHitGrpIndex)));
}


Value* RTBuilder::getLeafType(
    StackPointerVal* StackPointer, CallableShaderTypeMD ShaderTy)
{
    Value* Val = this->getHitInfoDWord(StackPointer, ShaderTy, VALUE_NAME("leafTypeDW"));
    Val = this->CreateLShr(
        Val, this->getInt32((uint32_t)MemHit::RT::Xe::Offset::leafType));

    Val = this->CreateAnd(
        Val,
        this->getInt32(BITMASK((uint32_t)MemHit::RT::Xe::Bits::leafType)),
        VALUE_NAME("leafType"));

    return Val;
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
    Module* module = this->GetInsertBlock()->getModule();
    Value* stackID = this->CreateCall(
        GenISAIntrinsic::getDeclaration(
            module, GenISAIntrinsic::GenISA_SyncStackID),
        None,
        VALUE_NAME("SyncStackID"));

    return stackID;
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

