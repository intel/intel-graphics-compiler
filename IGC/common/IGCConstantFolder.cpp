/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/IGCConstantFolder.h"
#include <cfenv>
#include "Probe/Assertion.h"
#include "Types.hpp"
#include "iStdLib/utility.h"

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
        return IGCLLVM::ConstantFolderBase::CreateFAdd(C0, C1);
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
        return IGCLLVM::ConstantFolderBase::CreateFMul(C0, C1);
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
    const llvm::fltSemantics& outputSemantics = dstType->isHalfTy() ? llvm::APFloatBase::IEEEhalf() :
        dstType->isFloatTy() ? llvm::APFloatBase::IEEEsingle() :
        llvm::APFloatBase::IEEEdouble();
    bool losesInfo = false;
    llvm::APFloat::opStatus status = APF.convert(outputSemantics, roundingMode, &losesInfo);
    if (llvm::APFloat::opInvalidOp != status)
    {
        return llvm::ConstantFP::get(C0->getContext(), APF);
    }
    else
    {
        return nullptr;
    }
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
