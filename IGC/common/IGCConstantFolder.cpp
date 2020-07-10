/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
#include "common/IGCConstantFolder.h"
#include <cfenv>

namespace IGC
{

IGCConstantFolder::IGCConstantFolder() :
    llvm::ConstantFolder()
{

}

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
    assert(llvm::isa<llvm::ConstantFP>(C0));
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    double C0value = C0->getType()->isFloatTy() ? static_cast<double>(APF.convertToFloat()) :
        APF.convertToDouble();
    if (C0value > 0.0)
    {
        return llvm::ConstantFP::get(C0->getType(), 1. / sqrt(C0value));
    }
    else
    {
        return nullptr;
    }
}

llvm::Constant* IGCConstantFolder::CreateRoundNE(llvm::Constant* C0) const
{
    assert(llvm::isa<llvm::ConstantFP>(C0));
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
    assert(llvm::isa<llvm::ConstantFP>(C0));
    auto APF = llvm::cast<llvm::ConstantFP>(C0)->getValueAPF();
    const llvm::APFloat& zero = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(C0->getType(), 0.))->getValueAPF();
    const llvm::APFloat& One = llvm::cast<llvm::ConstantFP>(llvm::ConstantFP::get(C0->getType(), 1.))->getValueAPF();
    return llvm::ConstantFP::get(C0->getContext(), llvm::minnum(One, llvm::maxnum(zero, APF)));
}

llvm::Constant* IGCConstantFolder::CreateFAdd(llvm::Constant* C0, llvm::Constant* C1, llvm::APFloatBase::roundingMode roundingMode) const
{
    if (llvm::isa<llvm::UndefValue>(C0) || llvm::isa<llvm::UndefValue>(C1))
    {
        return llvm::ConstantFolder::CreateFAdd(C0, C1);
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
        return llvm::ConstantFolder::CreateFMul(C0, C1);
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
        return llvm::ConstantFolder::CreateFPCast(C0, dstType);
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
    assert(llvm::isa<llvm::ConstantFP>(C0));
    if (llvm::cast<llvm::ConstantFP>(C0)->getValueAPF().isFinite())
    {
        return llvm::ConstantFP::get(C0->getType(), 0.0f);
    }
    else
    {
        // Preserve nan or infinite value
        return C0;
    }
}

} // namespace IGC
