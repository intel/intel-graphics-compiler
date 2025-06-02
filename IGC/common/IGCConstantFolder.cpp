/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/APFloat.h"
#include "common/LLVMWarningsPop.hpp"

#include "common/IGCConstantFolder.h"
#include <cfenv>
#include "Probe/Assertion.h"
#include "Types.hpp"
#include "iStdLib/utility.h"
#include <cmath>

namespace IGC
{

llvm::Constant* IGCConstantFolder::CreateGradientXFine(llvm::Constant* C0) const
{
    return CreateGradient(C0);
}

llvm::Constant* IGCConstantFolder::CreateGradientYFine(llvm::Constant* C0) const
{
    return CreateGradient(C0);
}

llvm::Constant* IGCConstantFolder::CreateGradientX(llvm::Constant* C0) const
{
    return CreateGradient(C0);
}

llvm::Constant* IGCConstantFolder::CreateGradientY(llvm::Constant* C0) const
{
    return CreateGradient(C0);
}

llvm::Constant* IGCConstantFolder::CreateRsq(llvm::Constant* C0) const
{
    IGC_ASSERT(nullptr != C0);
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    IGC_ASSERT(llvm::isa<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != llvm::cast<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != C0->getType());
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    double C0value = C0->getType()->isFloatTy() ? static_cast<double>(APF.convertToFloat()) :
        APF.convertToDouble();
    if (C0value > 0.0)
    {
        const double sq = sqrt(C0value);
        IGC_ASSERT(sq);
        return llvm::ConstantFP::get(C0->getType(), 1.0 / sq);
    }
    else
    {
        return nullptr;
    }
}

llvm::Constant* IGCConstantFolder::CreateRoundNE(llvm::Constant* C0) const
{
    IGC_ASSERT(nullptr != C0);
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    IGC_ASSERT(llvm::isa<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != llvm::cast<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != C0->getType());
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    double C0value = C0->getType()->isFloatTy() ? static_cast<double>(APF.convertToFloat()) :
        APF.convertToDouble();
    const int currentRoundingMode = std::fegetround();
    // Round to nearest, ties round to even.
    std::fesetround(FE_TONEAREST);
    double result = std::rint(C0value);
    std::fesetround(currentRoundingMode);
    return llvm::ConstantFP::get(C0->getType(), result);
}

llvm::Constant* IGCConstantFolder::CreateFSat(llvm::Constant* C0) const
{
    IGC_ASSERT(nullptr != C0);
    if (llvm::isa<llvm::UndefValue>(C0))
        return nullptr;
    IGC_ASSERT(llvm::isa<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != llvm::cast<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != C0->getType());
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    const llvm::APFloat& zero = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(C0->getType(), 0.))->getValueAPF();
    const llvm::APFloat& One = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(C0->getType(), 1.))->getValueAPF();
    return llvm::ConstantFP::get(C0->getContext(), llvm::minnum(One, llvm::maxnum(zero, APF)));
}

llvm::Constant* IGCConstantFolder::CreateFAdd(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1))
    {
        return IGCLLVM::ConstantFolderBase::CreateBinOp(llvm::Instruction::FAdd, C0, C1);
    }
    llvm::ConstantFP* CFP0 = llvm::cast<llvm::ConstantFP>(C0);
    llvm::ConstantFP* CFP1 = llvm::cast<llvm::ConstantFP>(C1);
    llvm::APFloat firstOperand = CFP0->getValueAPF();
    llvm::APFloat secondOperand = CFP1->getValueAPF();
    llvm::APFloat::opStatus status = firstOperand.add(secondOperand, roundingMode);
    if (llvm::APFloat::opInvalidOp != status)
    {
        return llvm::ConstantFP::get(C0->getContext(), firstOperand);
    }
    else
    {
        return nullptr;
    }
}

llvm::Constant* IGCConstantFolder::CreateFMul(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1))
    {
        return IGCLLVM::ConstantFolderBase::CreateBinOp(llvm::Instruction::FMul, C0, C1);
    }
    llvm::ConstantFP* CFP0 = llvm::cast<llvm::ConstantFP>(C0);
    llvm::ConstantFP* CFP1 = llvm::cast<llvm::ConstantFP>(C1);
    llvm::APFloat firstOperand = CFP0->getValueAPF();
    llvm::APFloat secondOperand = CFP1->getValueAPF();
    llvm::APFloat::opStatus status = firstOperand.multiply(secondOperand, roundingMode);
    if (llvm::APFloat::opInvalidOp != status)
    {
        return llvm::ConstantFP::get(C0->getContext(), firstOperand);
    }
    else
    {
        return nullptr;
    }
}

llvm::Constant* IGCConstantFolder::CreateFPTrunc(llvm::Constant* C0, llvm::Type* dstType, llvm::APFloatBase::roundingMode roundingMode) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return IGCLLVM::ConstantFolderBase::CreateFPCast(C0, dstType);
    }
    llvm::APFloat APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    bool losesInfo = false;
    llvm::APFloat::opStatus status = APF.convert(dstType->getFltSemantics(), roundingMode, &losesInfo);
    if (llvm::APFloat::opInvalidOp != status)
    {
        return llvm::ConstantFP::get(C0->getContext(), APF);
    }
    else
    {
        return nullptr;
    }
}

// Helper structure to describe a floating point type
// See llvm.org/doxygen/APFloat_8cpp_source.html
struct FloatSemantics
{
    /* The largest E such that 2^E is representable; this matches the
       definition of IEEE 754.  */
    int32_t maxExponent;

    /* The smallest E such that 2^E is a normalized number; this
       matches the definition of IEEE 754.  */
    int32_t minExponent;

    /* Number of bits in the significand.  This includes the integer
       bit.  */
    unsigned int precision;

    /* Number of bits actually used in the semantics. */
    unsigned int sizeInBits;

    /* Has no Inf, only NaN */
    bool hasNoInf = false;
};
// IEEE binary16 format
static constexpr FloatSemantics semHF  = { 15, -14, 11, 16 };
// Note: LLVM 16+ supports E5M2 and E4M3 types in APFloat
// E5M2 format
static constexpr FloatSemantics semBF8 = { 15, -14,  3,  8 };
// E4M3 format, has no Inf
static constexpr FloatSemantics semHF8 = { 8,   -6,  4,  8, true };

inline uint32_t Round(
    uint32_t man,
    uint32_t numLostBits,
    bool isNegative,
    uint32_t roundingMode)
{
    uint32_t lostBitsMask = BITMASK(numLostBits);
    uint32_t lostBits = man & lostBitsMask;
    uint32_t lostBitsHalfMinusOne = BITMASK(numLostBits - 1);
    uint32_t tieToEvenBias = (man & BIT(numLostBits)) >> numLostBits;

    switch (roundingMode)
    {
    case ROUND_TO_NEAREST_EVEN:
        man += lostBitsHalfMinusOne + tieToEvenBias;
        man >>= numLostBits;
        break;
    case ROUND_TO_NEGATIVE:
        man >>= numLostBits;
        if (lostBits != 0 && isNegative)
        {
            man += 1;
        }
        break;
    case ROUND_TO_POSITIVE:
        man >>= numLostBits;
        if (lostBits != 0 && !isNegative)
        {
            man += 1;
        }
        break;
    case ROUND_TO_ZERO:
        man >>= numLostBits;
        break;
    default:
        man >>= numLostBits;
        IGC_ASSERT_MESSAGE(0, "Unsupported rounding mode");
        break;
    }
    return man;
}

inline uint32_t ConvertFloat(
    uint32_t intVal,
    const FloatSemantics& srcSem,
    const FloatSemantics& dstSem,
    uint32_t roundingMode = ROUND_TO_NEAREST_EVEN,
    bool saturate = false)
{
    uint32_t srcNumManBits = srcSem.precision - 1;
    uint32_t srcNumExpBits = srcSem.sizeInBits - srcNumManBits - 1;
    uint32_t dstNumManBits = dstSem.precision - 1;
    uint32_t dstNumExpBits = dstSem.sizeInBits - dstNumManBits - 1;

    uint32_t expBits = (intVal >> srcNumManBits) & BITMASK(srcNumExpBits);
    uint32_t manBits = intVal & BITMASK(srcNumManBits);
    bool isNegative = (intVal & BIT(srcSem.sizeInBits - 1)) != 0;
    bool isPositive = !isNegative;
    bool isDenorm = expBits == 0 && manBits > 0;
    bool isZero = expBits == 0 && manBits == 0;
    bool isInf = srcSem.hasNoInf ? false : (expBits == BITMASK(srcNumExpBits) && manBits == 0);
    bool isNan = srcSem.hasNoInf ?
        ((BITMASK(srcSem.sizeInBits - 1) & intVal) == BITMASK(srcSem.sizeInBits - 1)) :
        (expBits == BITMASK(srcNumExpBits) && manBits != 0);

    int32_t srcExpBias = 1 - srcSem.minExponent;
    int32_t dstExpBias = 1 - dstSem.minExponent;
    // Calculate the exponent and mantissa
    int32_t exp = isDenorm ? srcSem.minExponent : (int_cast<int32_t>(expBits) - srcExpBias);
    // Add the implicit leading 1 for normal numbers.
    int32_t man = (isZero || isDenorm) ? manBits : (manBits | BIT(srcNumManBits));

    // Calculate special values for the destination format.
    uint32_t signVal = isNegative ? BIT(dstSem.sizeInBits - 1) : 0;
    uint32_t nanVal = signVal | BITMASK(dstSem.sizeInBits - 1);
    uint32_t infVal = signVal | (BITMASK(dstNumExpBits) << dstNumManBits);
    uint32_t maxVal = signVal | (BITMASK(dstSem.sizeInBits - 1) & ~BIT(dstNumManBits));
    if (dstSem.hasNoInf)
    {
        infVal = nanVal;
        // E4M3 max normal = S.1111.110
        maxVal = signVal | (BITMASK(dstSem.sizeInBits - 1) & ~1);
    }

    // Handle special cases
    if (isZero)
    {
        return signVal;
    }
    if (isInf)
    {
        return infVal;
    }
    if (isNan)
    {
        return nanVal;
    }

    // Normalize the mantissa
    while ((man & BIT(srcNumManBits)) == 0)
    {
        man <<= 1;
        exp--;
    }
    if (exp < dstSem.minExponent)
    {
        if (dstSem.minExponent - exp - 1 > int_cast<int32_t>(dstNumManBits))
        {
            // Underflow
            return signVal;
        }
        // Denorm
        int32_t dstManLsb = srcNumManBits - dstNumManBits + dstSem.minExponent - exp;
        if (dstManLsb > 0)
        {
            man = Round(man, dstManLsb, isNegative, roundingMode);
        }
        else
        {
            man <<= -dstManLsb;
        }
        return signVal | man;
    }
    // Remove the implicit leading 1.
    man &= ~BIT(srcNumManBits);
    if (dstNumManBits < srcNumManBits)
    {
        int32_t dstManLsb = srcNumManBits - dstNumManBits;
        man = Round(man, dstManLsb, isNegative, roundingMode);
        // Mantissa overflow
        if ((man & BIT(dstNumManBits)) != 0)
        {
            man = 0;
            exp++;
        }
    }
    else
    {
        man <<= (dstNumManBits - srcNumManBits);
    }
    // Overflow
    if (exp > dstSem.maxExponent)
    {
        if (roundingMode == ROUND_TO_NEGATIVE && isPositive)
        {
            return maxVal;
        }
        else if (roundingMode == ROUND_TO_POSITIVE && isNegative)
        {
            return maxVal;
        }
        else if (saturate)
        {
            return maxVal;
        }
        return infVal;
    }
    expBits = (exp + dstExpBias) << dstNumManBits;
    if (saturate && dstSem.hasNoInf && (expBits | man) > maxVal)
    {
        return maxVal;
    }
    return signVal | expBits | man;
}

llvm::Constant* IGCConstantFolder::CreateHFToBF8Trunc(llvm::Constant* C0, llvm::Type* dstType, uint32_t roundingMode, bool saturate) const
{
    IGC_ASSERT(dstType->isIntegerTy());
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return llvm::UndefValue::get(dstType);
    }
    llvm::APFloat APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    uint32_t intVal = int_cast<uint32_t>(APF.bitcastToAPInt().getZExtValue());
    intVal = ConvertFloat(intVal, semHF, semBF8, roundingMode, saturate);
    return llvm::ConstantInt::get(dstType, intVal);
}

llvm::Constant* IGCConstantFolder::CreateHFToHF8Trunc(llvm::Constant* C0, llvm::Type* dstType, uint32_t roundingMode, bool saturate) const
{
    IGC_ASSERT(dstType->isIntegerTy());
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return llvm::UndefValue::get(dstType);
    }
    llvm::APFloat APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    uint32_t intVal = int_cast<uint32_t>(APF.bitcastToAPInt().getZExtValue());
    intVal = ConvertFloat(intVal, semHF, semHF8, roundingMode, saturate);
    return llvm::ConstantInt::get(dstType, intVal);
}

llvm::Constant* IGCConstantFolder::CreateBF8ToHF(llvm::Constant* C0) const
{
    llvm::Type* halfTy = llvm::Type::getHalfTy(C0->getContext());
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return llvm::UndefValue::get(halfTy);
    }
    uint32_t intVal = int_cast<uint32_t>(llvm::cast<llvm::ConstantInt>(C0)->getZExtValue());
    intVal = ConvertFloat(intVal, semBF8, semHF);
    llvm::APFloat halfVal(halfTy->getFltSemantics(), llvm::APInt(16, intVal));
    return llvm::ConstantFP::get(halfTy, halfVal);
}

llvm::Constant* IGCConstantFolder::CreateHF8ToHF(llvm::Constant* C0) const
{
    llvm::Type* halfTy = llvm::Type::getHalfTy(C0->getContext());
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return llvm::UndefValue::get(halfTy);
    }
    uint32_t intVal = int_cast<uint32_t>(llvm::cast<llvm::ConstantInt>(C0)->getZExtValue());
    intVal = ConvertFloat(intVal, semHF8, semHF);
    llvm::APFloat halfVal(halfTy->getFltSemantics(), llvm::APInt(16, intVal));
    return llvm::ConstantFP::get(halfTy, halfVal);
}

llvm::Constant* IGCConstantFolder::CreateUbfe(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1) || llvm::isa<llvm::UndefValue>(C2))
    {
        return nullptr;
    }
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0); // width
    llvm::ConstantInt* CI1 = llvm::cast<llvm::ConstantInt>(C1); // offset
    llvm::ConstantInt* CI2 = llvm::cast<llvm::ConstantInt>(C2); // the number to shift
    uint32_t width = int_cast<uint32_t>(CI0->getZExtValue());
    uint32_t offset = int_cast<uint32_t>(CI1->getZExtValue());
    uint32_t bitwidth = CI2->getType()->getBitWidth();

    llvm::APInt result = CI2->getValue();
    if ((width + offset) < bitwidth)
    {
        result = result.shl(bitwidth - (width + offset));
        result = result.lshr(bitwidth - width);
    }
    else
    {
        // For HW only bits 0..4 in offset value are relevant
        result = result.lshr(offset & BITMASK_RANGE(0, 4));
    }
    return llvm::ConstantInt::get(C0->getContext(), result);
}

llvm::Constant* IGCConstantFolder::CreateIbfe(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1) || llvm::isa<llvm::UndefValue>(C2) || C2->getType()->getIntegerBitWidth() != 32)
    {
        return nullptr;
    }
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0); // width
    llvm::ConstantInt* CI1 = llvm::cast<llvm::ConstantInt>(C1); // offset
    llvm::ConstantInt* CI2 = llvm::cast<llvm::ConstantInt>(C2); // the number to shift
    uint32_t width = int_cast<uint32_t>(CI0->getZExtValue());
    uint32_t offset = int_cast<uint32_t>(CI1->getZExtValue());
    uint32_t bitwidth = CI2->getType()->getBitWidth();

    llvm::APInt result = CI2->getValue();
    if ((width + offset) < bitwidth)
    {
        result = result.shl(bitwidth - (width + offset));
        result = result.ashr(bitwidth - width);
    }
    else
    {
        // For HW only bits 0..4 in offset value are relevant
        result = result.ashr(offset & BITMASK_RANGE(0, 4));
    }
    return llvm::ConstantInt::get(C0->getContext(), result);
}

llvm::Constant* IGCConstantFolder::CreateCanonicalize(llvm::Constant* C0, bool flushDenorms /*= true*/) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return C0;
    }
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    if (flushDenorms && APF.isDenormal())
    {
        APF = llvm::APFloat::getZero(APF.getSemantics(), APF.isNegative());
    }
    return llvm::ConstantFP::get(C0->getContext(), APF);
}

llvm::Constant* IGCConstantFolder::CreateGradient(llvm::Constant* C0) const
{
    IGC_ASSERT(nullptr != C0);
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    IGC_ASSERT(llvm::isa<llvm::ConstantFP>(C0));
    IGC_ASSERT(nullptr != llvm::cast<llvm::ConstantFP>(C0));
    if (llvm::cast<llvm::ConstantFP>(C0)->getValueAPF().isFinite())
    {
        IGC_ASSERT(nullptr != C0->getType());
        return llvm::ConstantFP::get(C0->getType(), 0.0f);
    }
    else
    {
        // Preserve nan or infinite value
        return C0;
    }
}

llvm::Constant* IGCConstantFolder::CreateFirstBitHi(llvm::Constant* C0) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0);
    const unsigned fbh = CI0->getValue().countLeadingZeros();
    if (fbh == CI0->getType()->getBitWidth())
    {
        return llvm::ConstantInt::get(C0->getType(), -1);
    }
    return llvm::ConstantInt::get(C0->getType(), fbh);
}

llvm::Constant* IGCConstantFolder::CreateFirstBitShi(llvm::Constant* C0) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    IGC_ASSERT(llvm::isa<llvm::ConstantInt>(C0));
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0);
    const uint32_t fbs = CI0->isNegative() ? CI0->getValue().countLeadingOnes() : CI0->getValue().countLeadingZeros();
    if (fbs == CI0->getType()->getBitWidth())
    {
        return llvm::ConstantInt::get(C0->getType(), -1);
    }
    return llvm::ConstantInt::get(C0->getType(), fbs);
}

llvm::Constant* IGCConstantFolder::CreateFirstBitLo(llvm::Constant* C0) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    IGC_ASSERT(llvm::isa<llvm::ConstantInt>(C0));
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0);
    const unsigned fbl = CI0->getValue().countTrailingZeros();
    if (fbl == CI0->getType()->getBitWidth())
    {
        return llvm::ConstantInt::get(C0->getType(), -1);
    }
    return llvm::ConstantInt::get(C0->getType(), fbl);
}

llvm::Constant* IGCConstantFolder::CreateBfi(llvm::Constant* C0, llvm::Constant* C1, llvm::Constant* C2, llvm::Constant* C3) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1) || llvm::isa<llvm::UndefValue>(C2))
    {
        return nullptr;
    }
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0); // width
    llvm::ConstantInt* CI1 = llvm::cast<llvm::ConstantInt>(C1); // offset
    llvm::ConstantInt* CI2 = llvm::cast<llvm::ConstantInt>(C2); // the number the bits are taken from.
    llvm::ConstantInt* CI3 = llvm::cast<llvm::ConstantInt>(C3); // the number with bits to be replaced.
    uint32_t width = int_cast<uint32_t>(CI0->getZExtValue());
    uint32_t offset = int_cast<uint32_t>(CI1->getZExtValue());
    uint32_t bitwidth = CI2->getType()->getBitWidth();
    llvm::APInt bitmask = llvm::APInt::getBitsSet(bitwidth, offset, offset + width);
    llvm::APInt result = CI2->getValue();
    result = result.shl(offset);
    result = (result & bitmask) | (CI3->getValue() & ~bitmask);
    return llvm::ConstantInt::get(C0->getContext(), result);
}

llvm::Constant* IGCConstantFolder::CreateBfrev(llvm::Constant* C0) const
{
    if (llvm::isa<llvm::UndefValue>(C0))
    {
        return nullptr;
    }
    llvm::ConstantInt* CI0 = llvm::cast<llvm::ConstantInt>(C0);
    llvm::APInt result = CI0->getValue();
    result = result.reverseBits();
    return llvm::ConstantInt::get(C0->getContext(), result);
}

} // namespace IGC
