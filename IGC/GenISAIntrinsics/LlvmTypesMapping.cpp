/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "LlvmTypesMapping.h"
#include "StringMacros.hpp"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/DerivedTypes.h"


#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Type.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{

llvm::Type* VoidType::GetType(llvm::LLVMContext& ctx)
{
    return llvm::Type::getVoidTy(ctx);
}

bool VoidType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isVoidTy();
    return isCorrect;
}

llvm::Type* IntegerType::GetType(llvm::LLVMContext& ctx, uint8_t bitWidth)
{
    IGC_ASSERT(bitWidth != 0);
    return llvm::IntegerType::get(ctx, bitWidth);
}

llvm::Type* IntegerType::GetType(llvm::LLVMContext& ctx) const
{
    return GetType(ctx, m_BitWidth);
}

bool IntegerType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isIntegerTy();
    if (isCorrect)
    {
        if (m_BitWidth != 0)
        {
            llvm::IntegerType* pIntTy = llvm::cast<llvm::IntegerType>(pType);
            isCorrect = pIntTy->getBitWidth() == m_BitWidth;
        }
    }
    return isCorrect;
}

llvm::Type* FloatType::GetType(llvm::LLVMContext& ctx, uint8_t bitWidth)
{
    IGC_ASSERT(bitWidth != 0);
    switch (bitWidth)
    {
    case 16:
        return llvm::Type::getHalfTy(ctx);
    case 32:
        return llvm::Type::getFloatTy(ctx);
    default:
        IGC_ASSERT(bitWidth == 64);
        return llvm::Type::getDoubleTy(ctx);
    }
}

llvm::Type* FloatType::GetType(llvm::LLVMContext& ctx) const
{
    return GetType(ctx, m_BitWidth);
}

bool FloatType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isFloatingPointTy();
    if (isCorrect)
    {
        if (m_BitWidth != 0)
        {
            switch (m_BitWidth)
            {
            case 16:
                isCorrect = pType->isHalfTy();
                break;
            case 32:
                isCorrect = pType->isFloatTy();
                break;
            default:
                IGC_ASSERT(m_BitWidth == 64);
                isCorrect = pType->isDoubleTy();
                break;
            }
        }
    }
    return isCorrect;
}

llvm::Type* VectorType::GetType(llvm::LLVMContext& ctx, llvm::Type* pType, uint32_t numElements)
{
    IGC_ASSERT(numElements != 0 && pType != nullptr && !pType->isVoidTy());
    return IGCLLVM::FixedVectorType::get(pType, numElements);
}

llvm::Type* VectorType::GetType(llvm::LLVMContext& ctx) const
{
    llvm::Type* pElementType = m_Type.GetType(ctx);
    IGC_ASSERT(pElementType != nullptr);
    return GetType(ctx, pElementType, m_NumElements);
}

bool VectorType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isVectorTy();
    if (m_NumElements != 0)
    {
        isCorrect = isCorrect && m_NumElements == llvm::cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
    }
    isCorrect = isCorrect && m_Type.VerifyType(llvm::cast<IGCLLVM::FixedVectorType>(pType)->getElementType());
    return isCorrect;
}

llvm::Type* StructType::GetType(llvm::LLVMContext& ctx, llvm::ArrayRef<llvm::Type*> types)
{
    IGC_ASSERT(types.size() > 0);
    return llvm::StructType::get(ctx, types);
}

llvm::Type* StructType::GetType(llvm::LLVMContext& ctx) const
{
    std::vector<llvm::Type*> types;
    const MemberTypeListNode* pCurr = m_pMemberTypeList;
    while (pCurr != nullptr)
    {
        llvm::Type* pMemberType = pCurr->m_Type.GetType(ctx);
        types.push_back(pMemberType);
        pCurr = pCurr->m_pNext;
    }
    return GetType(ctx, types);
}

bool StructType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isStructTy();
    if (m_pMemberTypeList != nullptr)
    {
        uint32_t index = 0;
        const MemberTypeListNode* pCurr = m_pMemberTypeList;
        while (pCurr != nullptr && isCorrect)
        {
            isCorrect = pCurr->m_Type.VerifyType(llvm::cast<llvm::StructType>(pType)->getElementType(index++));
            pCurr = pCurr->m_pNext;
        }
    }
    return isCorrect;
}

llvm::Type* PointerType::GetType(llvm::LLVMContext& ctx, llvm::Type* pType, uint32_t addressSpace)
{
    IGC_ASSERT(pType != nullptr && addressSpace != UINT32_MAX);
    return llvm::PointerType::get(pType, addressSpace);
}

llvm::Type* PointerType::GetType(llvm::LLVMContext& ctx) const
{
    llvm::Type* pElementType = m_Type.GetType(ctx);
    IGC_ASSERT(pElementType != nullptr);
    return GetType(ctx, pElementType, m_AddressSpace);
}

bool PointerType::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = pType->isPointerTy();
    if (m_AddressSpace != UINT32_MAX)
    {
        isCorrect = isCorrect && m_AddressSpace == llvm::cast<llvm::PointerType>(pType)->getAddressSpace();
    }
    isCorrect = isCorrect && m_Type.VerifyType(IGCLLVM::getNonOpaquePtrEltTy(pType));
    return isCorrect;
}

llvm::Type* TypeDescription::GetType(llvm::LLVMContext& ctx) const
{
    llvm::Type* pResult = nullptr;
    Visit([&ctx, &pResult](const auto& currType) {
        using T = std::decay_t<decltype(currType)>;
        using Traits = typename T::Traits;
        if constexpr (Traits::scHasTypeDefinition)
        {
            pResult = currType.GetType(ctx);
        }
        if (pResult == nullptr)
        {
            if constexpr (Traits::scHasDefaultType)
            {
                const TypeDescription* pDefaultTypeDesc = currType.GetDefaultType();
                if (pDefaultTypeDesc != nullptr)
                {
                    pResult = pDefaultTypeDesc->GetType(ctx);
                }
            }
        }
        });
    return pResult;
}

bool TypeDescription::VerifyType(llvm::Type* pType) const
{
    bool isCorrect = true;
    Visit([&isCorrect, &pType](const auto& currType) {
        using T = std::decay_t<decltype(currType)>;
        using Traits = typename T::Traits;
        if constexpr (Traits::scHasTypeDefinition)
        {
            isCorrect = currType.VerifyType(pType);
        }
        });
    return isCorrect;
}

} // namespace IGC
