/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// A collection of useful classes/functions to be shared across passes.
///
//===----------------------------------------------------------------------===//

#include "Utils.h"

using namespace llvm;

namespace IGC {

UnifiedBits::UnifiedBits(unsigned NumBits) : KB(NumBits) {}

UnifiedBits& UnifiedBits::operator+=(Value* V)
{
    auto* CI = dyn_cast<ConstantInt>(V);
    if (!CI)
    {
        Unset = false;
        KB.resetAll();
        return *this;
    }

    IGC_ASSERT_MESSAGE(CI->getBitWidth() == KB.getBitWidth(), "must match!");

    APInt APRHS(CI->getBitWidth(), CI->getZExtValue());
    KnownBits RHS(CI->getBitWidth());

    RHS.One = APRHS;
    RHS.Zero = ~APRHS;

    if (Unset)
    {
        Unset = false;
        KB = RHS;
        return *this;
    }

    KB.Zero &= RHS.Zero;
    KB.One &= RHS.One;

    return *this;
}

llvm::Optional<bool> UnifiedBits::operator[](unsigned BitPosition) const
{
    if (KB.Zero[BitPosition])
        return false;

    if (KB.One[BitPosition])
        return true;

    return None;
}

const llvm::Optional<llvm::APInt> UnifiedBits::getConstant() const
{
    if (!KB.isConstant())
        return None;

    return KB.getConstant();
}

UnifiedBits examineRayFlags(const RayDispatchShaderContext& Ctx)
{
    UnifiedBits UB{ 32 };

    // If we can't see all the TraceRay()s, we have to conservatively say we
    // don't know anything about the rayflags values.
    if (IGC_IS_FLAG_ENABLED(DisableExamineRayFlag) || !Ctx.canWholeShaderCompile())
        return UB;

    visitGenIntrinsic(*Ctx.getModule(), GenISAIntrinsic::GenISA_TraceRayAsyncHL,
    [&](GenIntrinsicInst* GII) {
        auto* TRI = cast<TraceRayAsyncHLIntrinsic>(GII);
        UB += TRI->getFlag();
    });

    return UB;
}

bool collectAnalyzablePayloadUses(
    Value* I,
    const DataLayout& DL,
    SmallVector<PayloadUse, 4>& Uses,
    uint64_t Offset,
    uint32_t instTypeMask)
{
    IGC_ASSERT_EXIT_MESSAGE(isa<Instruction>(I) || isa<Argument>(I), "Invalid Value!");
    for (auto* U : I->users())
    {
        auto* UI = cast<Instruction>(U);
        switch (UI->getOpcode())
        {
        case Instruction::GetElementPtr:
        {
            auto* GEPI = cast<GetElementPtrInst>(UI);
            unsigned Width = DL.getIndexSizeInBits(
                GEPI->getPointerOperand()->getType()->getPointerAddressSpace());
            APInt BasePtrOffset(Width, 0);
            if (GEPI->accumulateConstantOffset(DL, BasePtrOffset))
            {
                uint64_t NewOffset = Offset + BasePtrOffset.getZExtValue();
                if (!collectAnalyzablePayloadUses(UI, DL, Uses, NewOffset, instTypeMask))
                    return false;
            }
            else
            {
                return false;
            }
            break;
        }
        case Instruction::BitCast:
            if (!collectAnalyzablePayloadUses(UI, DL, Uses, Offset, instTypeMask))
                return false;
            break;
        case Instruction::Load:
            if (instTypeMask & PL_Inst_Type::Load)
            {
                Uses.push_back({ UI, DL, Offset });
            }
            break;
        case Instruction::Store:
            if (I == cast<StoreInst>(UI)->getValueOperand())
                return false;
            if (instTypeMask & PL_Inst_Type::Store)
            {
                Uses.push_back({ UI, DL, Offset });
            }
            break;
        default:
            return false;
        }
    }

    return true;
}

unsigned gcd(unsigned Dividend, unsigned Divisor) {
    // Dividend and Divisor will be naturally swapped as needed.
    while (Divisor) {
        unsigned Rem = Dividend % Divisor;
        Dividend = Divisor;
        Divisor = Rem;
    };
    return Dividend;
}

} // namespace IGC
