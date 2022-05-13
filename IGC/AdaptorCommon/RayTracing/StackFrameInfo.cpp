/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "StackFrameInfo.h"
#include "MDFrameWork.h"
#include "RTBuilder.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

/*

Note [Stack Layout]

F1 stack start----> +----------------+   ^   (F1 = Stack Frame 1)
                    |                |   |
                    |    Alloca F1   |   |
                    |                |   |   F1 stack size
                    +----------------+   |
                    |                |   |
                    | Spilled var F1 |   |
                    |                |   |
F2 stack start----> +----------------+   v
                    |                |   ^
                    |  Return IP to  |   |
                    |  continuation  |   |
                    |                |   |   F2 stack size
                    +----------------+   |
                    |                |   |
                    |    Arguments   |   |
                    |                |   |
                    +----------------+   |
                    |                |   |
                    |    Alloca F2   |   |
                    |                |   |
                    +----------------+   |
                    |                |   |
                    | Spilled var F2 |   |
                    |                |   v
 F3 stack start---> +----------------+
                    |                |            Note [Argument Order]
                    | Return IP to   |        ----+-----------------+
                    | F2 continuation|       /    | Payload Pointer |
                    |                |      /     +-----------------+
                    +----------------+-----/      |    Hit Kind     | (procedural)
                    |                |            +-----------------+
                    |  Arguments to  |            | Custom Hit Attr | (procedural)
                    |  F3            |           /+-----------------+
                    +----------------+----------/

Note that this stack layout is purely a construct of the compiler.  The hardware
doesn't know anything about it.

*/

using namespace llvm;
using namespace IGC;
using namespace RTStackFormat;

static std::string getTypeRepr(const Type* Ty)
{
    std::string s;
    raw_string_ostream ss(s);
    Ty->print(ss);
    ss.flush();
    return s;
}

bool StackFrameInfo::skipRecording() const
{
    return (IGC_IS_FLAG_DISABLED(RayTracingDumpYaml) || !LogStackFrameEntries);
}

StackFrameInfo::StackFrameInfo(
    const Function *RootFunc,
    CallableShaderTypeMD ShaderType,
    Optional<HIT_GROUP_TYPE> HitGroupTy,
    RayDispatchShaderContext *RayCtx,
    const FunctionMetaData &FMD,
    RayTracingSWTypes& RTSWTypes,
    bool LogStackFrameEntries) :
    RTArgs(RootFunc, ShaderType, HitGroupTy, RayCtx, FMD, RTSWTypes, LogStackFrameEntries)
{
}

void StackFrameInfo::addFunction(const Function *F)
{
    addAllocas(F);
    addSpills(F);
    addFills(F);
}

bool StackFrameInfo::isContinuation(const Function* F) const
{
    return (F != RootFunction);
}

bool StackFrameInfo::isRoot(const Function* F) const
{
    return !isContinuation(F);
}

bool StackFrameInfo::isRayGen() const
{
    return (FuncType == RayGen);
}

bool StackFrameInfo::isRayGenRoot(const Function *F) const
{
    return (isRayGen() && isRoot(F));
}

uint32_t StackFrameInfo::getAllocasOffset() const
{
    // Raygen has no return IP since it is always the first frame on the
    // call stack so the allocas would be right at the start of the frame
    // in that case.
    if (FuncType == RayGen)
        return 0;

    return TraceRayRTArgs::getPayloadOffset() + ArgumentSize;
}

uint32_t StackFrameInfo::getSpillOffset() const
{
    return getAllocasOffset() + TotalAllocaSize;
}

uint32_t StackFrameInfo::getFrameSize() const
{
    uint32_t TotalSize = getSpillOffset() + TotalSpillSize;
    TotalSize = IGC::Align(TotalSize, StackFrameAlign);

    return TotalSize;
}

void StackFrameInfo::addAllocas(const Function *F)
{
    auto &DL = F->getParent()->getDataLayout();
    auto& C = F->getContext();

    uint64_t CurOffset = 0;
    uint32_t CurAllocaIdx = 0;

    SmallVector<Type*, 4> Tys;

    for (auto &I : instructions(*F))
    {
        if (auto *AI = dyn_cast<AllocaInst>(&I))
        {
            if (!RTBuilder::isNonLocalAlloca(AI))
                continue;

            uint32_t Offset = TotalAllocaSize;
            uint32_t TypeSize =
                (uint32_t)DL.getTypeAllocSize(AI->getAllocatedType());

            if (CurOffset != Offset)
            {
                IGC_ASSERT_MESSAGE((CurOffset < Offset), "bad offset!");
                // insert padding
                uint64_t Diff = Offset - CurOffset;
                auto* Padding = ArrayType::get(Type::getInt8Ty(C), Diff);
                Tys.push_back(Padding);
                CurAllocaIdx++;
            }

            Tys.push_back(AI->getAllocatedType());
            AllocaIdxMap[AI] = CurAllocaIdx++;
            CurOffset = Offset + TypeSize;

            recordAllocaEntry(AI, TypeSize);

            TotalAllocaSize += TypeSize;
        }
    }

    AllocaStructTy = Tys.empty() ?
        StructType::get(C, true) :
        StructType::create(C, Tys, "IGC::Allocas", true);
}

void StackFrameInfo::recordAllocaEntry(const AllocaInst* AI, uint32_t Size)
{
    if (skipRecording())
        return;

    StackFrameEntry Entry;
    Entry.Name = "N/A";
    if (AI->hasName())
        Entry.Name = AI->getName().str();
    Entry.Offset = TotalAllocaSize;
    Entry.TypeRepr = getTypeRepr(AI->getAllocatedType());
    Entry.EntryType = ENTRY_ALLOCA;
    Entry.Size = Size;
    AllocaEntries.push_back(Entry);
}

void StackFrameInfo::addFills(const Function* F)
{
    auto& DL = F->getParent()->getDataLayout();
    auto& C = F->getContext();
    uint64_t CurOffset = 0;
    uint32_t CurFillIdx = 0;

    SmallVector<const FillValueIntrinsic*, 4> Fills;
    for (auto& I : instructions(*F))
    {
        if (auto* II = dyn_cast<FillValueIntrinsic>(&I))
            Fills.push_back(II);
    }

    std::stable_sort(Fills.begin(), Fills.end(), [&](auto* A, auto* B) {
        return A->getOffset() < B->getOffset();
    });

    SmallVector<Type*, 4> Tys;
    DenseMap<uint64_t, uint32_t> OffsetToIdx;
    for (auto *FI : Fills)
    {
        uint64_t Offset = FI->getOffset();
        uint64_t TypeSize = DL.getTypeAllocSize(FI->getType());

        if (auto I = OffsetToIdx.find(Offset); I != OffsetToIdx.end())
        {
            FillIdxMap[FI] = I->second;
        }
        else
        {
            if (CurOffset != Offset)
            {
                IGC_ASSERT_MESSAGE((CurOffset < Offset), "bad offset!");
                // insert padding
                uint64_t Diff = Offset - CurOffset;
                auto* Padding = ArrayType::get(Type::getInt8Ty(C), Diff);
                Tys.push_back(Padding);
                CurFillIdx++;
            }
            OffsetToIdx[Offset] = CurFillIdx;

            Tys.push_back(FI->getType());
            FillIdxMap[FI] = CurFillIdx++;
            CurOffset = Offset + TypeSize;
        }
    }

    if (!Tys.empty())
    {
        auto *StructTy = StructType::create(C, Tys, "IGC::Fills", true);
        FillTyMap[F] = StructTy;
    }
}

void StackFrameInfo::addSpillsCompacted(const Function* F)
{
    auto& C = F->getContext();
    bool DoLog = !skipRecording();
    for (auto &BB : *F)
    {
        std::string ContName;
        std::vector<StackFrameEntry> Entries;

        uint64_t CurOffset = 0;
        uint32_t CurSpillIdx = 0;
        SmallVector<Type*, 4> Tys;
        for (auto &I : BB)
        {
            if (auto *II = dyn_cast<SpillValueIntrinsic>(&I))
            {
                uint64_t Offset = II->getOffset();
                uint64_t TypeSize = DL.getTypeAllocSize(
                    II->getData()->getType());

                if (CurOffset != Offset)
                {
                    IGC_ASSERT_MESSAGE((CurOffset < Offset), "bad offset!");
                    // insert padding
                    uint64_t Diff = Offset - CurOffset;
                    auto* Padding = ArrayType::get(Type::getInt8Ty(C), Diff);
                    Tys.push_back(Padding);
                    CurSpillIdx++;
                }

                Tys.push_back(II->getData()->getType());
                SpillIdxMap[II] = CurSpillIdx++;
                CurOffset = Offset + TypeSize;

                recordSpillUnionEntry(II, Entries, (uint32_t)TypeSize);

                TotalSpillSize = std::max(
                    TotalSpillSize,
                    (uint32_t)(Offset + TypeSize));
            }

            if (DoLog)
            {
                if (auto* CI = dyn_cast<ContinuationHLIntrinsic>(&I))
                    ContName = CI->getContinuationFn()->getName().str();
            }
        }

        if (!Tys.empty())
        {
            auto *StructTy = StructType::create(C, Tys, "IGC::Spills", true);
            SpillTyMap[&BB] = StructTy;
        }

        if (DoLog)
        {
            if (!ContName.empty())
            {
                StackFrameSpillUnion Union{ ContName, Entries };
                SpillEntries.push_back(Union);
            }
        }
    }
}

void StackFrameInfo::addSpillsUncompacted(const Function* F)
{
    auto& C = F->getContext();

    SmallVector<const SpillValueIntrinsic*, 4> Spills;
    for (auto& I : instructions(*F))
    {
        if (auto* II = dyn_cast<SpillValueIntrinsic>(&I))
            Spills.push_back(II);
    }

    std::stable_sort(Spills.begin(), Spills.end(), [&](auto* A, auto* B) {
        return A->getOffset() < B->getOffset();
    });

    uint64_t CurOffset = 0;
    uint32_t CurSpillIdx = 0;
    SmallVector<Type*, 4> Tys;
    DenseMap<uint64_t, uint32_t> OffsetToIdx;
    for (auto* II : Spills)
    {
        uint64_t Offset = II->getOffset();
        uint64_t TypeSize = DL.getTypeAllocSize(
            II->getData()->getType());

        if (auto I = OffsetToIdx.find(Offset); I != OffsetToIdx.end())
        {
            SpillIdxMap[II] = I->second;
        }
        else
        {
            if (CurOffset != Offset)
            {
                IGC_ASSERT_MESSAGE((CurOffset < Offset), "bad offset!");
                // insert padding
                uint64_t Diff = Offset - CurOffset;
                auto* Padding = ArrayType::get(Type::getInt8Ty(C), Diff);
                Tys.push_back(Padding);
                CurSpillIdx++;
            }
            OffsetToIdx[Offset] = CurSpillIdx;

            Tys.push_back(II->getData()->getType());
            SpillIdxMap[II] = CurSpillIdx++;
            CurOffset = Offset + TypeSize;

            TotalSpillSize = std::max(
                TotalSpillSize,
                (uint32_t)(Offset + TypeSize));
        }
    }

    if (!Tys.empty())
    {
        auto* StructTy = StructType::create(C, Tys, "IGC::Spills", true);
        for (auto& BB : *F)
        {
            SpillTyMap[&BB] = StructTy;
        }
    }
}

void StackFrameInfo::addSpills(const Function *F)
{
    if (IGC_IS_FLAG_ENABLED(DisableCompactifySpills))
        addSpillsUncompacted(F);
    else
        addSpillsCompacted(F);
}

void StackFrameInfo::recordSpillUnionEntry(
    const llvm::SpillValueIntrinsic* SV,
    std::vector<StackFrameEntry>& Entries,
    uint32_t Size)
{
    if (skipRecording())
        return;

    StackFrameEntry Entry;
    Entry.Name = "N/A";
    if (SV->getData()->hasName())
        Entry.Name = SV->getData()->getName().str();
    Entry.Offset = (uint32_t)SV->getOffset();
    Entry.TypeRepr = getTypeRepr(SV->getData()->getType());
    Entry.EntryType = ENTRY_SPILL;
    Entry.Size = Size;
    Entries.push_back(Entry);
}

Value* StackFrameInfo::getSpillPtr(
    IRBuilder<>& IRB,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const SpillValueIntrinsic* SI,
    const Twine& Name) const
{
    auto* BB = SI->getParent();
    auto I = SpillTyMap.find(BB);
    IGC_ASSERT_MESSAGE(I != SpillTyMap.end(), "missing?");
    auto* PtrTy = I->second->getPointerTo(SWStackAddrSpace);

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, PtrTy, VALUE_NAME("&Frame"));

    auto IdxI = SpillIdxMap.find(SI);
    IGC_ASSERT_MESSAGE(IdxI != SpillIdxMap.end(), "missing?");

    Value* Indices[] = {
        IRB.getInt32(0),
        IRB.getInt32(*SpillSlot),
        IRB.getInt32(IdxI->second)
    };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, Name);
}

Value* StackFrameInfo::getFillPtr(
    IRBuilder<>& IRB,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const FillValueIntrinsic* FI,
    const Twine& Name) const
{
    auto I = FillTyMap.find(FI->getFunction());
    IGC_ASSERT_MESSAGE(I != FillTyMap.end(), "missing?");
    auto* PtrTy = I->second->getPointerTo(SWStackAddrSpace);

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, PtrTy, VALUE_NAME("&Frame"));

    auto IdxI = FillIdxMap.find(FI);
    IGC_ASSERT_MESSAGE(IdxI != FillIdxMap.end(), "missing?");

    Value* Indices[] = {
        IRB.getInt32(0),
        IRB.getInt32(*SpillSlot),
        IRB.getInt32(IdxI->second)
    };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, Name);
}

Value* StackFrameInfo::getAllocaPtr(
    IRBuilder<>& IRB,
    RTBuilder::SWStackPtrVal* FrameAddr,
    const AllocaInst* AI,
    const Twine& Name) const
{
    auto* PtrTy = AllocaStructTy->getPointerTo(SWStackAddrSpace);

    auto* Ptr = IRB.CreateBitOrPointerCast(
        FrameAddr, PtrTy, VALUE_NAME("&Frame"));

    auto IdxI = AllocaIdxMap.find(AI);
    IGC_ASSERT_MESSAGE(IdxI != AllocaIdxMap.end(), "missing?");

    Value* Indices[] = {
        IRB.getInt32(0),
        IRB.getInt32(*AllocaSlot),
        IRB.getInt32(IdxI->second)
    };

    return IRB.CreateInBoundsGEP(nullptr, Ptr, Indices, Name);
}

void StackFrameInfo::finalize()
{
    // Create all the whole frame types (See Note [Stack Layout])
    ArgumentSlot = 0;
    AllocaSlot   = 1;
    SpillSlot    = 2;

    auto& C = RootFunction->getContext();

    Type* ArgumentTy = nullptr;
    if (FuncType == RayGen)
    {
        // No args for raygen.  Stub out an empty type.
        ArgumentTy = StructType::get(C, true);
    }
    else
    {
        ArgumentTy = getArgumentType();
    }

    Type* CurAllocaStructTy = AllocaStructTy;
    auto mkSpillType = [&](StructType* SpillTy)
    {
        SmallVector<Type*, 4> Tys{
            ArgumentTy,
            CurAllocaStructTy,
            SpillTy
        };

        auto* Ty = StructType::create(
            RootFunction->getContext(), Tys, "IGC::Frame", true);

        return Ty;
    };

    auto mkAllocaType = [&](StructType* AllocaTy)
    {
        SmallVector<Type*, 4> Tys{
            ArgumentTy,
            AllocaTy,
        };

        auto* Ty = StructType::create(
            RootFunction->getContext(), Tys, "IGC::Frame", true);

        return Ty;
    };

    AllocaStructTy = mkAllocaType(AllocaStructTy);

    RTSWTypes.FullFrameTys.push_back(AllocaStructTy);

    // Update the spill map with the whole struct
    for (auto& [BB, SpillTy] : SpillTyMap)
    {
        auto* FrameTy = mkSpillType(SpillTy);
        IGC_ASSERT(nullptr != BB);
        IGC_ASSERT_MESSAGE(BB->getModule()->getDataLayout().getTypeAllocSize(FrameTy) <= getFrameSize(), "too big?");
        SpillTyMap[BB] = FrameTy;
        RTSWTypes.FullFrameTys.push_back(FrameTy);
    }

    // Update the spill map with the whole struct
    for (auto& [F, FillTy] : FillTyMap)
    {
        auto* FrameTy = mkSpillType(FillTy);
        IGC_ASSERT(nullptr != F);
        IGC_ASSERT_MESSAGE(F->getParent()->getDataLayout().getTypeAllocSize(FrameTy) <= getFrameSize(), "too big?");
        FillTyMap[F] = FrameTy;
        RTSWTypes.FullFrameTys.push_back(FrameTy);
    }

    // The below code is for debugging purposes.
    if (skipRecording())
        return;

    // patch up the offsets from all data in the stack frame and write it out
    // to metadata.  The metadata will be read at the end of compilation to
    // dump this information out to the *output.yaml file.
    for (auto& Alloca : AllocaEntries)
        Alloca.Offset += getAllocasOffset();

    for (auto& UnionEntry : SpillEntries)
    {
        for (auto& Entry : UnionEntry.Entries)
            Entry.Offset += getSpillOffset();
    }

    ModuleMetaData* ModMD = Ctx.getModuleMetaData();
    auto& FuncMD = ModMD->FuncMD;

    auto Entry = FuncMD.find(const_cast<Function*>(RootFunction));
    IGC_ASSERT_MESSAGE((Entry != FuncMD.end()), "Missing metadata?");

    auto &rtInfo = Entry->second.rtInfo;

    // Raygen shaders doen't have a return IP because there's nothing to return
    // to once raygen is done.
    if (FuncType != RayGen)
    {
        StackFrameEntry RetIP;
        RetIP.Name = "Return IP";
        RetIP.Offset = TraceRayRTArgs::getReturnIPOffset();
        // represent that we may have a pointer or an ID there for now.
        RetIP.TypeRepr = "i8 addrspace(1)* | i64";
        RetIP.EntryType = ENTRY_RETURN_IP;
        RetIP.Size = sizeof(TraceRayRTArgs::ReturnIP);

        rtInfo.Entries.push_back(RetIP);
    }

    for (auto& Arg : ArgumentEntries)
        rtInfo.Entries.push_back(Arg);

    for (auto& Alloca : AllocaEntries)
        rtInfo.Entries.push_back(Alloca);

    rtInfo.SpillUnions = SpillEntries;
}
