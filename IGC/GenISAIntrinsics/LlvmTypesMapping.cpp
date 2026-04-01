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

#include <algorithm>

namespace IGC {

/// Resolve a ValueBound to a concrete uint32_t using resolvedTypes for PropertyRef bounds.
static uint32_t ResolveBound(const ValueBound &bound, llvm::ArrayRef<llvm::Type *> resolvedTypes) {
  if (bound.IsLiteral())
    return bound.m_Value;
  IGC_ASSERT_MESSAGE(bound.IsRef(), "ValueBound must be Literal or PropertyRef");
  uint8_t refIndex = bound.m_Ref.m_Index;
  IGC_ASSERT_MESSAGE(refIndex < resolvedTypes.size() && resolvedTypes[refIndex] != nullptr,
                     "PropertyRef references an unresolved type");
  llvm::Type *pType = resolvedTypes[refIndex];
  switch (bound.m_Ref.m_Extraction) {
  case PropertyExtractionKind::VectorNumElements:
    IGC_ASSERT_MESSAGE(pType->isVectorTy(), "VectorNumElements requires a vector type");
    return llvm::cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
  case PropertyExtractionKind::IntegerBitWidth:
    IGC_ASSERT_MESSAGE(pType->isIntegerTy(), "IntegerBitWidth requires an integer type");
    return llvm::cast<llvm::IntegerType>(pType)->getBitWidth();
  }
  return 0;
}

/// Resolve a ValueConstraint to an exact uint32_t value using resolvedTypes.
static uint32_t ResolveConstraintExact(const ValueConstraint &vc, llvm::ArrayRef<llvm::Type *> resolvedTypes) {
  if (vc.IsExact())
    return vc.GetExact();
  uint32_t low = ResolveBound(vc.m_Low, resolvedTypes);
  uint32_t high = ResolveBound(vc.m_High, resolvedTypes);
  IGC_ASSERT_MESSAGE(low == high && low != 0,
                     "GetType requires constraint bounds to resolve to the same non-zero value");
  return low;
}

llvm::Type *VoidType::GetType(llvm::LLVMContext &ctx) { return llvm::Type::getVoidTy(ctx); }

bool VoidType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isVoidTy();
  return isCorrect;
}

llvm::Type *IntegerType::GetType(llvm::LLVMContext &ctx, uint8_t bitWidth) {
  IGC_ASSERT(bitWidth != 0);
  return llvm::IntegerType::get(ctx, bitWidth);
}

llvm::Type *IntegerType::GetType(llvm::LLVMContext &ctx) const {
  IGC_ASSERT_MESSAGE(m_BitWidth.IsExact(), "GetType requires an exact bit-width (not a range or unbounded).");
  return GetType(ctx, static_cast<uint8_t>(m_BitWidth.GetExact()));
}

llvm::Type *IntegerType::GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const {
  if (m_BitWidth.IsExact())
    return GetType(ctx, static_cast<uint8_t>(m_BitWidth.GetExact()));
  uint32_t bitWidth = ResolveConstraintExact(m_BitWidth, resCtx.m_OverloadedTypes);
  return GetType(ctx, static_cast<uint8_t>(bitWidth));
}

bool IntegerType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isIntegerTy();
  if (!isCorrect) {
    return false;
  }
  uint32_t actual = llvm::cast<llvm::IntegerType>(pType)->getBitWidth();
  if (m_BitWidth.IsExact()) {
    isCorrect = (actual == m_BitWidth.GetExact());
  } else if (!m_BitWidth.IsAny()) {
    const auto &low = m_BitWidth.GetLow();
    const auto &high = m_BitWidth.GetHigh();
    if (low.IsLiteral() && low.m_Value != 0) {
      isCorrect = actual >= low.m_Value;
    }
    if (isCorrect && high.IsLiteral() && high.m_Value != 0) {
      isCorrect = actual <= high.m_Value;
    }
  }
  return isCorrect;
}

llvm::Type *FloatType::GetType(llvm::LLVMContext &ctx, uint8_t bitWidth) {
  IGC_ASSERT(bitWidth != 0);
  switch (bitWidth) {
  case 16:
    return llvm::Type::getHalfTy(ctx);
  case 32:
    return llvm::Type::getFloatTy(ctx);
  default:
    IGC_ASSERT(bitWidth == 64);
    return llvm::Type::getDoubleTy(ctx);
  }
}

llvm::Type *FloatType::GetType(llvm::LLVMContext &ctx) const { return GetType(ctx, m_BitWidth); }

bool FloatType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isFloatingPointTy();
  if (isCorrect) {
    if (m_BitWidth != 0) {
      switch (m_BitWidth) {
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

llvm::Type *VectorType::GetType(llvm::LLVMContext &ctx, llvm::Type *pType, uint32_t numElements) {
  IGC_ASSERT(numElements != 0 && pType != nullptr && !pType->isVoidTy());
  return IGCLLVM::FixedVectorType::get(pType, numElements);
}

llvm::Type *VectorType::GetType(llvm::LLVMContext &ctx) const {
  llvm::Type *pElementType = m_Type.GetType(ctx);
  IGC_ASSERT(pElementType != nullptr);
  IGC_ASSERT_MESSAGE(m_NumElements.IsExact(), "GetType requires an exact element count (not a range or unbounded).");
  return GetType(ctx, pElementType, m_NumElements.GetExact());
}

llvm::Type *VectorType::GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const {
  llvm::Type *pElementType = m_Type.GetType(ctx, resCtx);
  IGC_ASSERT(pElementType != nullptr);
  uint32_t numElements;
  if (m_NumElements.IsExact()) {
    numElements = m_NumElements.GetExact();
  } else {
    numElements = ResolveConstraintExact(m_NumElements, resCtx.m_OverloadedTypes);
  }
  return GetType(ctx, pElementType, numElements);
}

bool VectorType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isVectorTy();
  if (!isCorrect) {
    return false;
  }
  auto *pFixedVecTy = llvm::cast<IGCLLVM::FixedVectorType>(pType);
  uint32_t actual = pFixedVecTy->getNumElements();
  if (m_NumElements.IsExact()) {
    isCorrect = (actual == m_NumElements.GetExact());
  } else if (!m_NumElements.IsAny()) {
    const auto &low = m_NumElements.GetLow();
    const auto &high = m_NumElements.GetHigh();
    if (low.IsLiteral() && low.m_Value != 0) {
      isCorrect = actual >= low.m_Value;
    }
    if (isCorrect && high.IsLiteral() && high.m_Value != 0) {
      isCorrect = actual <= high.m_Value;
    }
  }
  isCorrect = isCorrect && m_Type.VerifyType(pFixedVecTy->getElementType());
  return isCorrect;
}

llvm::Type *StructType::GetType(llvm::LLVMContext &ctx, llvm::ArrayRef<llvm::Type *> types) {
  IGC_ASSERT(types.size() > 0);
  return llvm::StructType::get(ctx, types);
}

llvm::Type *StructType::GetType(llvm::LLVMContext &ctx) const {
  IGC_ASSERT(m_pMemberTypes != nullptr && m_NumMembers > 0);
  std::vector<llvm::Type *> types;
  types.reserve(m_NumMembers);
  for (uint32_t i = 0; i < m_NumMembers; ++i) {
    types.push_back(m_pMemberTypes[i].GetType(ctx));
  }
  return GetType(ctx, types);
}

llvm::Type *StructType::GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const {
  IGC_ASSERT(m_pMemberTypes != nullptr && m_NumMembers > 0);
  std::vector<llvm::Type *> types;
  types.reserve(m_NumMembers);
  for (uint32_t i = 0; i < m_NumMembers; ++i) {
    types.push_back(m_pMemberTypes[i].GetType(ctx, resCtx));
  }
  return GetType(ctx, types);
}

bool StructType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isStructTy();
  if (m_pMemberTypes != nullptr && m_NumMembers > 0) {
    auto *pStructTy = llvm::cast<llvm::StructType>(pType);
    for (uint32_t i = 0; i < m_NumMembers && isCorrect; ++i) {
      isCorrect = m_pMemberTypes[i].VerifyType(pStructTy->getElementType(i));
    }
  }
  return isCorrect;
}

llvm::Type *PointerType::GetType(llvm::LLVMContext &ctx, llvm::Type *pType, uint32_t addressSpace) {
  IGC_ASSERT(pType != nullptr && addressSpace != UINT32_MAX);
  return llvm::PointerType::get(pType, addressSpace);
}

llvm::Type *PointerType::GetType(llvm::LLVMContext &ctx) const {
  llvm::Type *pElementType = m_Type.GetType(ctx);
  IGC_ASSERT(pElementType != nullptr);
  return GetType(ctx, pElementType, m_AddressSpace);
}

llvm::Type *PointerType::GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const {
  llvm::Type *pElementType = m_Type.GetType(ctx, resCtx);
  IGC_ASSERT(pElementType != nullptr);
  return GetType(ctx, pElementType, m_AddressSpace);
}

bool PointerType::VerifyType(llvm::Type *pType) const {
  bool isCorrect = pType->isPointerTy();
  if (m_AddressSpace != UINT32_MAX) {
    isCorrect = isCorrect && m_AddressSpace == llvm::cast<llvm::PointerType>(pType)->getAddressSpace();
  }
  if (!IGCLLVM::isPointerTy(pType)) {
    isCorrect =
        isCorrect && m_Type.VerifyType(IGCLLVM::getNonOpaquePtrEltTy(pType)); // Legacy code: getNonOpaquePtrEltTy
  }
  return isCorrect;
}

bool TypeListType::VerifyType(llvm::Type *pType) const {
  if (m_pTypes == nullptr || m_NumTypes == 0) {
    return true;
  }
  return std::any_of(m_pTypes, m_pTypes + m_NumTypes,
                     [pType](const TypeDescription &desc) { return desc.VerifyType(pType); });
}

llvm::Type *TypeDescription::GetType(llvm::LLVMContext &ctx) const {
  llvm::Type *pResult = nullptr;
  Visit([&ctx, &pResult](const auto &currType) {
    using T = std::decay_t<decltype(currType)>;
    using Traits = typename T::Traits;
    if constexpr (Traits::scHasTypeDefinition) {
      pResult = currType.GetType(ctx);
    }
    if (pResult == nullptr) {
      if constexpr (Traits::scHasDefaultType) {
        const TypeDescription *pDefaultTypeDesc = currType.GetDefaultType();
        if (pDefaultTypeDesc != nullptr) {
          pResult = pDefaultTypeDesc->GetType(ctx);
        }
      }
    }
  });
  return pResult;
}

llvm::Type *TypeDescription::GetType(llvm::LLVMContext &ctx, TypeResolutionContext &resCtx) const {
  llvm::Type *pResult = nullptr;
  switch (m_ID) {
  case TypeID::ArgumentReference: {
    uint8_t refIndex = m_Reference.m_Index;
    if (refIndex < resCtx.m_OverloadedTypes.size() && resCtx.m_OverloadedTypes[refIndex] != nullptr) {
      pResult = resCtx.m_OverloadedTypes[refIndex];
    }
    if (pResult != nullptr) {
      switch (m_Reference.m_Extraction) {
      case ReferenceExtractionType::VectorElement:
        IGC_ASSERT_MESSAGE(pResult->isVectorTy(), "Referenced type must be a vector for VectorElement extraction");
        pResult = llvm::cast<IGCLLVM::FixedVectorType>(pResult)->getElementType();
        break;
      case ReferenceExtractionType::None:
      default:
        break;
      }
    }
    break;
  }
  case TypeID::Integer:
    if (m_Integer.IsOverloadable()) {
      pResult = resCtx.ConsumeOverloadedType();
    }
    if (pResult == nullptr) {
      pResult = m_Integer.GetType(ctx, resCtx);
    }
    break;
  case TypeID::Float:
    if (m_Float.IsOverloadable()) {
      pResult = resCtx.ConsumeOverloadedType();
    }
    if (pResult == nullptr) {
      pResult = m_Float.GetType(ctx);
    }
    break;
  case TypeID::Vector:
    if (m_Vector.IsOverloadable()) {
      pResult = resCtx.ConsumeOverloadedType();
    }
    if (pResult == nullptr) {
      pResult = m_Vector.GetType(ctx, resCtx);
    }
    break;
  case TypeID::Pointer:
    if (m_Pointer.IsOverloadable()) {
      pResult = resCtx.ConsumeOverloadedType();
    }
    if (pResult == nullptr) {
      pResult = m_Pointer.GetType(ctx, resCtx);
    }
    break;
  case TypeID::Struct:
    if (m_Struct.IsOverloadable()) {
      pResult = resCtx.ConsumeOverloadedType();
    }
    if (pResult == nullptr) {
      pResult = m_Struct.GetType(ctx, resCtx);
    }
    break;
  case TypeID::Any: {
    pResult = resCtx.ConsumeOverloadedType();
    if (pResult == nullptr) {
      const TypeDescription *pDefault = m_Any.GetDefaultType();
      if (pDefault != nullptr) {
        pResult = pDefault->GetType(ctx, resCtx);
      }
    }
    break;
  }
  case TypeID::TypeList: {
    pResult = resCtx.ConsumeOverloadedType();
    break;
  }
  default:
    pResult = GetType(ctx);
    break;
  }
  return pResult;
}

bool TypeDescription::VerifyType(llvm::Type *pType) const {
  bool isCorrect = true;
  Visit([&isCorrect, &pType](const auto &currType) {
    using T = std::decay_t<decltype(currType)>;
    using Traits = typename T::Traits;
    if constexpr (Traits::scHasTypeDefinition) {
      isCorrect = currType.VerifyType(pType);
    } else if constexpr (Traits::scTypeID == TypeID::TypeList) {
      isCorrect = currType.VerifyType(pType);
    }
  });
  return isCorrect;
}

} // namespace IGC
