/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// This contains a collection of methods to inspect the positions/existence
/// of arguments in a shader as well as methods to compute the types of the
/// argument portion of a stack frame.
///
//===----------------------------------------------------------------------===//

#include "RTStackFormat.h"
#include "RTArgs.h"
#include "MDFrameWork.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "Compiler/CISACodeGen/getCacheOpts.h"

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

void ArgQuery::init(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD)
{
    ShaderTy = FuncType;
    uint32_t Idx = 0;
    switch (FuncType)
    {
    case RayGen:
    case Intersection:
    case CallStackHandler:
        break;
    case Miss:
        if (FMD.rtInfo.hasTraceRayPayload)
            TraceRayPayloadIdx = Idx++;
        break;
    case Callable:
        if (FMD.rtInfo.hasCallableData)
            CallableShaderPayloadIdx = Idx++;
        break;
    case ClosestHit:
    // For any-hit shaders, there are two cases:
    // 1. procedural hit-group: call will be inlined and will use no
    // stack (args just passed as normal function args).
    // 2. triangle hit-group: ray payload passed, hit attributes will
    // come from potentialHit portion of stack.
    // TODO: in the procedural case we should have already deleted
    // the shader by this point.
    case AnyHit:
        if (FMD.rtInfo.hasTraceRayPayload)
            TraceRayPayloadIdx = Idx++;
        if (FMD.rtInfo.hasHitAttributes)
            HitAttributeIdx    = Idx++;
        break;
    default:
        IGC_ASSERT_MESSAGE(0, "unknown func type!");
        break;
    }
}

ArgQuery::ArgQuery(const Function& F, const CodeGenContext& Ctx)
{
    auto* MMD = Ctx.getModuleMetaData();
    auto& FuncMD = MMD->FuncMD;
    auto I = FuncMD.find(const_cast<Function*>(&F));
    IGC_ASSERT_MESSAGE(I != FuncMD.end(), "Missing metadata?");
    auto& FMD = I->second;

    init(FMD.rtInfo.callableShaderType, FMD);
}

ArgQuery::ArgQuery(const FunctionMetaData& FMD)
{
    init(FMD.rtInfo.callableShaderType, FMD);
}

ArgQuery::ArgQuery(CallableShaderTypeMD FuncType, const FunctionMetaData& FMD)
{
    init(FuncType, FMD);
}

Argument* ArgQuery::getPayloadArg(const Function* F) const
{
    return const_cast<Argument*>(getArg(F, getPayloadArgNo()));
}

Argument* ArgQuery::getHitAttribArg(const Function* F) const
{
    return const_cast<Argument*>(getArg(F, getHitAttribArgNo()));
}

Optional<uint32_t> ArgQuery::getPayloadArgNo() const
{
    if (ShaderTy == Callable)
        return CallableShaderPayloadIdx;
    else
        return TraceRayPayloadIdx;

    return None;
}

const Argument* ArgQuery::getArg(
    const Function* F,
    Optional<uint32_t> ArgNo) const
{
    // Not specified
    if (!ArgNo)
        return nullptr;

    auto* Arg = F->arg_begin();
    if (F->arg_size() <= *ArgNo)
        return nullptr;

    std::advance(Arg, *ArgNo);

    return Arg;
}

Optional<uint32_t> ArgQuery::getHitAttribArgNo() const
{
    return HitAttributeIdx;
}

RTArgs::RTArgs(
    const Function *RootFunc,
    CallableShaderTypeMD FuncType,
    Optional<HIT_GROUP_TYPE> HitGroupTy,
    RayDispatchShaderContext* Ctx,
    const FunctionMetaData& FMD,
    RayTracingSWTypes &RTSWTypes,
    bool LogStackFrameEntries) :
        TraceRayRTArgs(
            *Ctx,
            RTSWTypes,
            RootFunc->getParent()->getDataLayout()),
        RootFunction(RootFunc),
        FuncType(FuncType),
        FMD(FMD),
        LogStackFrameEntries(LogStackFrameEntries),
        HitGroupTy(HitGroupTy),
        Args(FuncType, FMD)
{
    addArguments();
}

TraceRayRTArgs::TraceRayRTArgs(
    RayDispatchShaderContext &Ctx,
    RayTracingSWTypes& RTSWTypes,
    const DataLayout &DL) :
    Ctx(Ctx),
    RTSWTypes(RTSWTypes),
    DL(DL)
{
    SWStackAddrSpace = RTBuilder::getSWStackAddrSpace(*Ctx.getModuleMetaData());
}

TraceRayRTArgs::TypeCacheTy& TraceRayRTArgs::getCache()
{
    if (!ExistingStructs.empty())
        return ExistingStructs;

    for (auto* Ty : RTSWTypes.FrameStartTys)
    {
        // should contain fields for return IP and payload.
        IGC_ASSERT_MESSAGE(Ty->getNumElements() == 2, "format change?");
        ExistingStructs[cast<PointerType>(Ty->getContainedType(1))] = Ty;
    }

    return ExistingStructs;
}

// Determine if the payload pointer needs to be padded out to avoid a partial
// write
bool TraceRayRTArgs::needPayloadPadding() const
{
    LSC_L1_L3_CC cacheOpts = SWStackStorePolicy(Ctx);

    // If we aren't L1 caching the SWStack, no padding needed
    if (cacheOpts == LSC_L1UC_L3UC || cacheOpts == LSC_L1UC_L3C_WB)
        return false;

    return (DL.getPointerSize(SWStackAddrSpace) != sizeof(PointerSize));
}

PointerType* TraceRayRTArgs::getType(PointerType* PayloadTy)
{
    IGC_ASSERT_MESSAGE(getReturnIPOffset() == 0, "changed?");
    IGC_ASSERT_MESSAGE(getPayloadOffset() == 8, "changed?");

    static_assert(ReturnIPSlot == 0);
    static_assert(PayloadSlot == 1);
    static_assert(PayloadPaddingSlot == 2);

    auto& Structs = getCache();

    auto I = Structs.find(PayloadTy);

    if (I != Structs.end())
        return I->second->getPointerTo(SWStackAddrSpace);

    auto& C = PayloadTy->getContext();

    SmallVector<Type*, 4> Tys {
        Type::getInt64Ty(C),
        PayloadTy
    };

    if (needPayloadPadding())
        Tys.push_back(Type::getInt32Ty(C));

    auto *STy = StructType::create(C, Tys, VALUE_NAME("IGC::FrameStart"), true);
    Structs.insert(std::make_pair(PayloadTy, STy));

    RTSWTypes.FrameStartTys.push_back(STy);

    return STy->getPointerTo(SWStackAddrSpace);
}

Value* TraceRayRTArgs::getReturnIPPtr(
    IRBuilder<>& IRB,
    Type* PayloadTy,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const Twine &FrameName)
{
    auto* Ty = getType(cast<PointerType>(PayloadTy));

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, Ty, FrameName);

    Value* Indices[] = { IRB.getInt32(0), IRB.getInt32(ReturnIPSlot) };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, VALUE_NAME("&ReturnIP"));
}

Value* TraceRayRTArgs::getPayloadPtr(
    IRBuilder<>& IRB,
    Type *PayloadTy,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const Twine &FrameName)
{
    auto* Ty = getType(cast<PointerType>(PayloadTy));

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, Ty, FrameName);

    Value* Indices[] = { IRB.getInt32(0), IRB.getInt32(PayloadSlot) };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, VALUE_NAME("&Payload"));
}

Value* TraceRayRTArgs::getPayloadPaddingPtr(
    IRBuilder<>& IRB,
    Type *PayloadTy,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const Twine &FrameName)
{
    auto* Ty = getType(cast<PointerType>(PayloadTy));

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, Ty, FrameName);

    Value* Indices[] = { IRB.getInt32(0), IRB.getInt32(PayloadPaddingSlot) };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, VALUE_NAME("&PayloadPad"));
}

RTArgs::TypeCacheTy& RTArgs::getCache()
{
    if (!ExistingStructs.empty())
        return ExistingStructs;

    for (auto* Ty : RTSWTypes.ArgumentTys)
    {
        // Should contain at least return IP and payload.  If procedural,
        // may also have hitkind and custom hit attr.
        IGC_ASSERT_MESSAGE(Ty->getNumElements() >= 2, "format change?");
        Type* CustomHitAttrTy = nullptr;
        if (CustomHitAttrSlot)
            CustomHitAttrTy = Ty->getContainedType(*CustomHitAttrSlot);

        ExistingStructs[
            std::make_pair(
                cast<PointerType>(Ty->getContainedType(PayloadSlot)),
                CustomHitAttrTy)] = Ty;
    }

    return ExistingStructs;
}

StructType* RTArgs::getArgumentType(Type *CustomHitAttrTy)
{
    IGC_ASSERT_MESSAGE(getReturnIPOffset() == 0, "changed?");
    IGC_ASSERT_MESSAGE(getPayloadOffset() == 8, "changed?");

    auto& Structs = getCache();

    PointerType* PayloadTy = cast<PointerType>(ArgTys[PayloadSlot]);
    Type* FoundCustomHitAttrTy = nullptr;

    SmallVector<Type*, 4> Tys = ArgTys;
    if (CustomHitAttrTy)
    {
        IGC_ASSERT_MESSAGE(CustomHitAttrSlot.hasValue(), "Not emitted?");
        FoundCustomHitAttrTy = CustomHitAttrTy;
        Tys[*CustomHitAttrSlot] = CustomHitAttrTy;
    }
    else if (CustomHitAttrSlot)
    {
        FoundCustomHitAttrTy = ArgTys[*CustomHitAttrSlot];
    }

    auto I = Structs.find(std::make_pair(PayloadTy, FoundCustomHitAttrTy));

    if (I != Structs.end())
        return I->second;

    auto& C = PayloadTy->getContext();

    auto *STy = StructType::create(C, Tys, VALUE_NAME("IGC::Arguments"), true);
    Structs.insert(
        std::make_pair(std::make_pair(PayloadTy, FoundCustomHitAttrTy), STy));

    RTSWTypes.ArgumentTys.push_back(STy);

    return STy;
}

Value* RTArgs::getCustomHitAttribPtr(
    IRBuilder<> &IRB,
    RTBuilder::SWStackPtrVal *FrameAddr,
    Type *CustomHitAttrTy)
{
    IGC_ASSERT_MESSAGE(isProcedural(), "not procedural?");

    auto* Ty = getArgumentType(CustomHitAttrTy)->getPointerTo(SWStackAddrSpace);

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, Ty, VALUE_NAME("&Arguments"));

    Value* Indices[] = { IRB.getInt32(0), IRB.getInt32(*CustomHitAttrSlot) };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, VALUE_NAME("&CustomHitAttr"));
}

Value* RTArgs::getHitKindPtr(
    IRBuilder<>& IRB, RTBuilder::SWStackPtrVal* FrameAddr)
{
    IGC_ASSERT_MESSAGE(isProcedural(), "not procedural?");

    auto* Ty = getArgumentType()->getPointerTo(SWStackAddrSpace);

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, Ty, VALUE_NAME("&Arguments"));

    Value* Indices[] = { IRB.getInt32(0), IRB.getInt32(*HitKindSlot) };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, VALUE_NAME("&HitKind"));
}

Argument* RTArgs::getPayloadArg(const Function* F) const
{
    return Args.getPayloadArg(F);
}

Argument* RTArgs::getHitAttribArg(const Function* F) const
{
    return Args.getHitAttribArg(F);
}

bool RTArgs::isProcedural() const
{
    return *HitGroupTy == HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE;
}

uint32_t TraceRayRTArgs::getReturnIPOffset()
{
    return 0;
}

uint32_t TraceRayRTArgs::getPayloadOffset()
{
    return sizeof(ReturnIP);
}

uint32_t RTArgs::getHitKindOffset() const
{
    // Note: the any-hit shader does have access to HitKind(), but it either:
    // 1) pulls from the RTStack in the triangle case
    // 2) for procedural, it is handled by inlining in the intersection.
    // IGC_ASSERT(FuncType == Intersection || FuncType == ClosestHit);

    // IGC_ASSERT(HitTy == RTStackFormat::HIT_GROUP_TYPE::PROCEDURAL_PRIMITIVE);

    return TraceRayRTArgs::getPayloadOffset() + sizeof(TraceRayRTArgs::PointerSize);
}

uint32_t RTArgs::getCustomHitAttrOffset() const
{
    return getHitKindOffset() + sizeof(uint32_t);
}

void RTArgs::addArguments()
{
    // We allocate a slot on the stack for the payload pointer even if this
    // shader doesn't specify it.  When doing a TraceRay(), we don't which
    // shader will be invoked.
    //
    // TODO: if we see that all shaders in the RTPSO skipped the payload arg
    // then we could do away with this.
    if (FuncType == RayGen)
        return;

    auto& C = RootFunction->getContext();

    uint32_t CurSlot = 0;

    // Return IP
    IGC_ASSERT(ReturnIPSlot == CurSlot);
    CurSlot++;
    ArgTys.push_back(Type::getInt64Ty(C));

    IGC_ASSERT(PayloadSlot == CurSlot);
    CurSlot++;
    if (auto *Arg = getPayloadArg(RootFunction))
    {
        ArgTys.push_back(Arg->getType());
    }
    else
    {
        ArgTys.push_back(Type::getInt8PtrTy(C, SWStackAddrSpace));
    }

    constexpr uint32_t PointerSize = sizeof(TraceRayRTArgs::PointerSize);

    if (needPayloadPadding())
    {
        // add padding so we don't have partial writes
        IGC_ASSERT(PayloadPaddingSlot == CurSlot);
        CurSlot++;
        ArgTys.push_back(Type::getInt32Ty(C));
    }

    // See Note [Argument Order] for a visual layout

    // payload
    ArgumentSize += PointerSize;
    recordArgEntry(
        "Payload",
        "i8*",
        PointerSize,
        TraceRayRTArgs::getPayloadOffset());

    if (FuncType == Miss || FuncType == Callable || FuncType == CallStackHandler)
        return;

    // Hit kind (if necessary)
    // TODO: this can be optimized by examining if there are any calls to
    // HitKind() in the closest-hit shader and not generating this stack slot
    // if it is not used.
    if (isProcedural())
    {
        HitKindSlot = CurSlot++;
        ArgTys.push_back(Type::getInt32Ty(C));
        ArgumentSize += sizeof(DWORD);
        recordArgEntry(
            "HitKind",
            "i32",
            sizeof(DWORD),
            getHitKindOffset());

        ArgumentSize += FMD.rtInfo.CustomHitAttrSizeInBytes;
        if (FMD.rtInfo.CustomHitAttrSizeInBytes != 0)
        {
            CustomHitAttrSlot = CurSlot++;
            if (auto * Arg = getHitAttribArg(RootFunction))
            {
                ArgTys.push_back(Arg->getType()->getPointerElementType());
            }
            else
            {
                auto* ArrayTy = ArrayType::get(
                    Type::getInt8Ty(C), FMD.rtInfo.CustomHitAttrSizeInBytes);
                ArgTys.push_back(ArrayTy);
            }
            recordArgEntry(
                "CustomHitAttr",
                "N/A",
                FMD.rtInfo.CustomHitAttrSizeInBytes,
                getCustomHitAttrOffset());
        }
    }
}

void RTArgs::recordArgEntry(
    const std::string& Name,
    const std::string& TypeRepr,
    uint32_t Size,
    uint32_t Offset)
{
    if (IGC_IS_FLAG_DISABLED(ShaderDumpEnable) || !LogStackFrameEntries)
        return;

    StackFrameEntry Entry;
    Entry.Name = Name;
    Entry.TypeRepr = TypeRepr;
    Entry.Offset = Offset;
    Entry.EntryType = ENTRY_ARGUMENT;
    Entry.Size = Size;
    ArgumentEntries.push_back(Entry);
}

