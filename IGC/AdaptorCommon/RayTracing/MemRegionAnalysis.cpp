/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Provides a utility to scan up a few instructions looking for a base address
/// corresponding to a region in raytracing memory.  This is to be reused
/// in whatever passes need info on what memory regions they are touching.
///
//===----------------------------------------------------------------------===//

#include "Compiler/CodeGenPublic.h"
#include "MemRegionAnalysis.h"
#include "MDFrameWork.h"
#include "Probe/Assertion.h"
#include "API/RayDispatchGlobalData.h"
#include "llvmWrapper/IR/Value.h"
#include <optional>

using namespace llvm;
using namespace IGC;

static bool isGlobalPtr(const InlineDataIntrinsic *I, const IGC::ModuleMetaData& moduleMetaData)
{
    uint32_t globalPtrOffset = 0;

    if (moduleMetaData.rtInfo.GlobalDataStyle == RayDispatchInlinedDataStyle::Xe)
    {
        constexpr uint32_t GlobalPtrOffset =
            offsetof(RayDispatchInlinedData, RayDispatchGlobalDataPtr) / sizeof(uint64_t);
        globalPtrOffset = GlobalPtrOffset;
    }

    return (I->getArg() == globalPtrOffset);
}

static std::optional<RTMemRegion> getIntrinsicRegion(
    const GenIntrinsicInst* GII,
    const IGC::ModuleMetaData& moduleMetaData)
{
    switch (GII->getIntrinsicID())
    {
    case GenISAIntrinsic::GenISA_SWHotZonePtr:
        return RTMemRegion::SWHotZone;
    case GenISAIntrinsic::GenISA_GlobalBufferPointer:
        return RTMemRegion::RTGlobals;
    case GenISAIntrinsic::GenISA_LocalBufferPointer:
        return RTMemRegion::LocalArgs;
    case GenISAIntrinsic::GenISA_AsyncStackPtr:
    case GenISAIntrinsic::GenISA_AsyncStackPtrPlaceHolder:
        return RTMemRegion::RTAsyncStack;
    case GenISAIntrinsic::GenISA_SyncStackPtr:
        return RTMemRegion::RTSyncStack;
    case GenISAIntrinsic::GenISA_SWStackPtr:
    case GenISAIntrinsic::GenISA_ContinuationSignpost:
        return RTMemRegion::SWStack;
    case GenISAIntrinsic::GenISA_InlinedData:
        if (isGlobalPtr(cast<InlineDataIntrinsic>(GII), moduleMetaData))
            return RTMemRegion::RTGlobals;
        else
            return std::nullopt;
    default:
        return std::nullopt;
    }
}

namespace IGC {

std::optional<RTMemRegion>
getRTRegionByAddrspace(const Value* V, const ModuleMetaData &MMD)
{
    auto* PtrTy = dyn_cast<PointerType>(V->getType());
    if (!PtrTy)
        return std::nullopt;

    uint32_t Addrspace = PtrTy->getPointerAddressSpace();

    auto& rtInfo = MMD.rtInfo;

    if (Addrspace == rtInfo.RTAsyncStackAddrspace)
        return RTMemRegion::RTAsyncStack;
    else if (Addrspace == rtInfo.SWHotZoneAddrspace)
        return RTMemRegion::SWHotZone;
    else if (Addrspace == rtInfo.SWStackAddrspace)
        return RTMemRegion::SWStack;
    else if (Addrspace == rtInfo.RTSyncStackAddrspace)
        return RTMemRegion::RTSyncStack;

    return std::nullopt;
}

std::optional<RTMemRegion> getRegionOffset(
    const Value* Ptr,
    const IGC::ModuleMetaData& moduleMetaData,
    const DataLayout *DL,
    uint64_t* Offset,
    uint64_t* dereferenceable_value)
{
    // Set an initial value for the Offset, overwriting whatever garbage value there may be.
    // If there is a getelementptr instruction the value of the *Offset will be updated.
    if (Offset) {
        *Offset = 0;
    }

    while (Ptr)
    {
        if (auto* GEPI = dyn_cast<GetElementPtrInst>(Ptr))
        {
            Ptr = GEPI->getPointerOperand();

            if (Offset)
            {
                unsigned IdxWidth = DL->getIndexSizeInBits(
                    Ptr->getType()->getPointerAddressSpace());
                APInt BasePtrOffset(IdxWidth, 0);
                // If there is a constant offset on the getelementptr instruction like i64 4
                // we can evaluate it at compile time
                if (GEPI->accumulateConstantOffset(*DL, BasePtrOffset)) {
                    *Offset += BasePtrOffset.getZExtValue();
                }
                // If there is a non-constant or variable offset on the getelementptr instruction like i64 %x
                // we cannot evaluate it at compile time, only at run time
                else {
                    return std::nullopt;
                }
            }
        }
        else if (auto* BCI = dyn_cast<BitCastInst>(Ptr))
        {
            Ptr = BCI->getOperand(0);
        }
        else if (auto *GII = dyn_cast<GenIntrinsicInst>(Ptr))
        {
            if (dereferenceable_value) {
                bool CanBeNull = false;
                bool CanBeFreed = false;
                *dereferenceable_value = IGCLLVM::getPointerDereferenceableBytes(Ptr, *DL, CanBeNull, CanBeFreed);
                // We probably only want to set the *dereferenceable_value if CanBeNull is false
                // But CanBeNull should never be true if you're calling this function on any of those intrinsics in getRegionOffset()
                IGC_ASSERT(!CanBeNull);
            }

            return getIntrinsicRegion(GII, moduleMetaData);
        }
        else
        {
            Ptr = nullptr;
        }
    }

    return std::nullopt;
}

// This is conceptually the same idea as getRegionOffset() but it doesn't
// compute offsets and is intended to be used late in compilation after GEPs
// have been lowered (or at least mostly lowered).  It thus looks through
// more constructs that we don't care to look at in earlier stages.
//
// Currently, the only user of this is for late stage LSC cache controls
// determination.
std::optional<RTMemRegion> getRTRegion(const Value* V, const ModuleMetaData &MMD)
{
    if (auto Region = getRTRegionByAddrspace(V, MMD))
        return Region;

    while (V)
    {
        auto* I = dyn_cast<Instruction>(V);
        if (!I)
            return std::nullopt;

        switch (I->getOpcode())
        {
        case Instruction::GetElementPtr:
        case Instruction::BitCast:
        case Instruction::IntToPtr:
        case Instruction::PtrToInt:
        case Instruction::Add:
        case Instruction::Or:
            V = I->getOperand(0);
            break;
        case Instruction::Call:
            if (auto* GII = dyn_cast<GenIntrinsicInst>(I))
                return getIntrinsicRegion(GII, MMD);
            return std::nullopt;
        default:
            return std::nullopt;
        }
    }

    return std::nullopt;
}

} // namespace IGC

