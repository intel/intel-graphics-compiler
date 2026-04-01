/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenIntrinsicFunctions.h"
#include "GenIntrinsicDefinition.h"
#include "GenIntrinsicLookup.h"
#include "Probe/Assertion.h"
#include "Compiler/CISACodeGen/helper.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Module.h"
#include "llvmWrapper/IR/Attributes.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Operator.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/CodeGen/ValueTypes.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

namespace {

#ifndef NDEBUG

/// Extract a scalar property from a resolved LLVM type.
uint32_t ResolveProperty(llvm::Type *pType, PropertyExtractionKind extraction) {
  switch (extraction) {
  case PropertyExtractionKind::VectorNumElements:
    IGC_ASSERT_MESSAGE(pType && pType->isVectorTy(), "VectorNumElements requires a vector type");
    return llvm::cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
  case PropertyExtractionKind::IntegerBitWidth:
    IGC_ASSERT_MESSAGE(pType && pType->isIntegerTy(), "IntegerBitWidth requires an integer type");
    return pType->getIntegerBitWidth();
  }
  return 0;
}

/// Resolve a ValueBound to a concrete uint32_t given the resolved types array.
/// Returns 0 for unbounded literals.
uint32_t ResolveBound(const ValueBound &bound, const llvm::ArrayRef<llvm::Type *> resolvedTypes) {
  if (bound.IsLiteral())
    return bound.m_Value;
  IGC_ASSERT_MESSAGE(bound.m_Ref.m_Index < resolvedTypes.size(), "PropertyRef index out of range");
  return ResolveProperty(resolvedTypes[bound.m_Ref.m_Index], bound.m_Ref.m_Extraction);
}

/// Check that actualValue satisfies the constraint, resolving any PropertyRef
/// bounds against resolvedTypes. Returns true if valid.
bool VerifyConstraint(uint32_t actualValue, const ValueConstraint &constraint,
                      const llvm::ArrayRef<llvm::Type *> resolvedTypes) {
  // Only verify constraints that contain at least one PropertyRef bound;
  // literal-only constraints are already handled by VerifyType().
  if (!constraint.m_Low.IsRef() && !constraint.m_High.IsRef())
    return true;

  uint32_t low = ResolveBound(constraint.m_Low, resolvedTypes);
  uint32_t high = ResolveBound(constraint.m_High, resolvedTypes);

  if (low != 0 && actualValue < low)
    return false;
  if (high != 0 && actualValue > high)
    return false;
  return true;
}

/// Verify cross-argument constraints for a single resolved type against its
/// TypeDescription. Only checks constraints involving PropertyRef bounds.
void VerifyCrossArgConstraints([[maybe_unused]] uint8_t index, const TypeDescription &typeDef, llvm::Type *pType,
                               const llvm::ArrayRef<llvm::Type *> resolvedTypes, const char *funcName) {
  if (!pType)
    return;

  switch (typeDef.m_ID) {
  case TypeID::Vector: {
    const ValueConstraint &numElem = typeDef.m_Vector.m_NumElements;
    if (numElem.m_Low.IsRef() || numElem.m_High.IsRef()) {
      IGC_ASSERT_MESSAGE(pType->isVectorTy(), "%s[%d]: Expected vector type for cross-arg constraint", funcName, index);
      uint32_t actualCount = llvm::cast<IGCLLVM::FixedVectorType>(pType)->getNumElements();
      IGC_ASSERT_MESSAGE(VerifyConstraint(actualCount, numElem, resolvedTypes),
                         "%s[%d]: Vector element count (%u) violates cross-argument constraint", funcName, index,
                         actualCount);
    }
    // Verify element type against the inner type description
    if (pType->isVectorTy()) {
      llvm::Type *elemTy = llvm::cast<llvm::VectorType>(pType)->getElementType();
      const TypeDescription &innerType = typeDef.m_Vector.m_Type;
      if (innerType.m_ID == TypeID::Integer) {
        const ValueConstraint &bw = innerType.m_Integer.m_BitWidth;
        if (bw.m_Low.IsRef() || bw.m_High.IsRef()) {
          IGC_ASSERT_MESSAGE(elemTy->isIntegerTy(),
                             "%s[%d]: Vector element must be integer for "
                             "bit-width cross-arg constraint",
                             funcName, index);
          uint32_t actualBits = elemTy->getIntegerBitWidth();
          IGC_ASSERT_MESSAGE(VerifyConstraint(actualBits, bw, resolvedTypes),
                             "%s[%d]: Vector element bit-width (%u) violates cross-argument "
                             "constraint",
                             funcName, index, actualBits);
        }
      }
    }
    break;
  }
  case TypeID::Integer: {
    const ValueConstraint &bw = typeDef.m_Integer.m_BitWidth;
    if (bw.m_Low.IsRef() || bw.m_High.IsRef()) {
      IGC_ASSERT_MESSAGE(pType->isIntegerTy(), "%s[%d]: Expected integer type for cross-arg constraint", funcName,
                         index);
      uint32_t actualBits = pType->getIntegerBitWidth();
      IGC_ASSERT_MESSAGE(VerifyConstraint(actualBits, bw, resolvedTypes),
                         "%s[%d]: Integer bit-width (%u) violates cross-argument constraint", funcName, index,
                         actualBits);
    }
    break;
  }
  default:
    break;
  }
}

#endif // NDEBUG

} // unnamed namespace

constexpr uint32_t scBeginIntrinsicIndex = static_cast<uint32_t>(llvm::GenISAIntrinsic::ID::no_intrinsic) + 1;
constexpr uint32_t scNumIntrinsics =
    static_cast<uint32_t>(llvm::GenISAIntrinsic::ID::num_genisa_intrinsics) - scBeginIntrinsicIndex;

/// Returns a stable mangling for the type specified for use in the name
/// mangling scheme used by 'any' types in intrinsic signatures.  The mangling
/// of named types is simply their name.  Manglings for unnamed types consist
/// of a prefix ('p' for pointers, 'a' for arrays, 'f_' for functions)
/// combined with the mangling of their component types.  A vararg function
/// type will have a suffix of 'vararg'.  Since function types can contain
/// other function types, we close a function type mangling with suffix 'f'
/// which can't be confused with it's prefix.  This ensures we don't have
/// collisions between two unrelated function types. Otherwise, you might
/// parse ffXX as f(fXX) or f(fX)X.  (X is a placeholder for any other type.)
std::string getMangledTypeStr(llvm::Type *Ty) {
  IGC_ASSERT(Ty);
  std::string Result;
  if (llvm::PointerType *PTyp = llvm::dyn_cast<llvm::PointerType>(Ty)) {
    Result += "p" + llvm::utostr(PTyp->getAddressSpace());

    // backward compatibility
    IGC::RESOURCE_DIMENSION_TYPE resDimTypeId = IGC::DecodeAS4GFXResourceType(PTyp->getAddressSpace());
    if (resDimTypeId != IGC::RESOURCE_DIMENSION_TYPE::NUM_RESOURCE_DIMENSION_TYPES) {
      Result += IGC::ResourceDimensionTypeName[resDimTypeId];
    } else if (!IGCLLVM::isPointerTy(PTyp)) {
      Result += getMangledTypeStr(IGCLLVM::getNonOpaquePtrEltTy(PTyp)); // Legacy code: getNonOpaquePtrEltTy
    }
  } else if (llvm::ArrayType *ATyp = llvm::dyn_cast<llvm::ArrayType>(Ty)) {
    Result += "a" + llvm::utostr(ATyp->getNumElements()) + getMangledTypeStr(ATyp->getElementType());
  } else if (llvm::StructType *STyp = llvm::dyn_cast<llvm::StructType>(Ty)) {
    if (!STyp->isLiteral())
      Result += STyp->getName();
    else {
      Result += "s" + llvm::utostr(STyp->getNumElements());
      for (unsigned int i = 0; i < STyp->getNumElements(); i++)
        Result += getMangledTypeStr(STyp->getElementType(i));
    }
  } else if (llvm::FunctionType *FT = llvm::dyn_cast<llvm::FunctionType>(Ty)) {
    Result += "f_" + getMangledTypeStr(FT->getReturnType());
    for (size_t i = 0; i < FT->getNumParams(); i++)
      Result += getMangledTypeStr(FT->getParamType(i));
    if (FT->isVarArg())
      Result += "vararg";
    // Ensure nested function types are distinguishable.
    Result += "f";
  } else if (llvm::isa<llvm::VectorType>(Ty))
    Result += "v" + llvm::utostr(llvm::cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements()) +
              getMangledTypeStr(llvm::cast<llvm::VectorType>(Ty)->getElementType());
  else
    Result += llvm::EVT::getEVT(Ty).getEVTString();
  return Result;
}

template <llvm::GenISAIntrinsic::ID id> class IntrinsicFunctionImp : public llvm::Function {
public:
  static constexpr llvm::GenISAIntrinsic::ID scID = id;
  using IntrinsicDefinitionT = IntrinsicDefinition<scID>;
  using Argument = typename IntrinsicDefinitionT::Argument;

  static bool classof(const llvm::Value *pValue) {
    return llvm::isa<llvm::Function>(pValue) && llvm::isa<IntrinsicFunctionImp<id>>(llvm::cast<llvm::Function>(pValue));
  }

  static bool classof(const llvm::Function *pFunc) {
    if (pFunc != nullptr) {
      return llvm::GenISAIntrinsic::getIntrinsicID(pFunc) != llvm::GenISAIntrinsic::ID::no_intrinsic;
    }
    return false;
  }

  static IntrinsicFunctionImp<id> *Get(llvm::Module &module, const llvm::ArrayRef<llvm::Type *> &overloadedTypes,
                                       const llvm::ArrayRef<llvm::Type *> &overloadedPointeeTys) {
    return llvm::cast<IntrinsicFunctionImp<id>>(GetDeclaration(module, overloadedTypes, overloadedPointeeTys));
  }

  static llvm::Function *GetDeclaration(llvm::Module &module, const llvm::ArrayRef<llvm::Type *> &overloadedTypes,
                                        const llvm::ArrayRef<llvm::Type *> &overloadedPointeeTys) {
    return GetOrInsert(module, overloadedTypes, overloadedPointeeTys);
  }

  static std::string GetName(const llvm::ArrayRef<llvm::Type *> &overloadedTypes,
                             const llvm::ArrayRef<llvm::Type *> &overloadedPointeeTys) {
    std::string result = IntrinsicDefinitionT::scFunctionRootName;
    for (unsigned i = 0; i < overloadedTypes.size(); ++i) {
      result += "." + getMangledTypeStr(overloadedTypes[i]);
    }
    for (unsigned i = 0; i < overloadedPointeeTys.size(); ++i) {
      result += "." + getMangledTypeStr(overloadedPointeeTys[i]);
    }
    return result;
  }

  static llvm::GenISAIntrinsic::IntrinsicComments GetIntrinsicComments() {
    llvm::GenISAIntrinsic::IntrinsicComments result = {};
    result.funcDescription = IntrinsicDefinitionT::scMainComment;
    result.outputs = {IntrinsicDefinitionT::scResultComment};
    if constexpr (static_cast<uint32_t>(IntrinsicDefinitionT::Argument::Count) > 0) {
      std::transform(IntrinsicDefinitionT::scArgumentComments.begin(), IntrinsicDefinitionT::scArgumentComments.end(),
                     std::back_inserter(result.inputs), [](const auto &comment) { return comment; });
    }
    return result;
  }

private:
  static llvm::Function *GetOrInsert(llvm::Module &module, const llvm::ArrayRef<llvm::Type *> &overloadedTypes,
                                     const llvm::ArrayRef<llvm::Type *> &overloadedPointeeTys) {
    llvm::LLVMContext &ctx = module.getContext();
    std::string funcName = GetName(overloadedTypes, overloadedPointeeTys);
    llvm::FunctionType *pFuncType = GetType(ctx, overloadedTypes);
    llvm::AttributeList attribs = GetAttributeList(module, overloadedPointeeTys);
    // There can never be multiple globals with the same name of different types,
    // because intrinsics must be a specific type.
    IGCLLVM::Module &M = static_cast<IGCLLVM::Module &>(module);
    llvm::Value *func = M.getOrInsertFunction(funcName, pFuncType, attribs);
    llvm::Function *pFunc = nullptr;
    if (llvm::isa<llvm::Function>(func))
      pFunc = llvm::cast<llvm::Function>(func);
    else if (llvm::isa<llvm::BitCastOperator>(func)) {
      llvm::BitCastOperator *bco = llvm::cast<llvm::BitCastOperator>(func);
      pFunc = llvm::cast<llvm::Function>(bco->getOperand(0));
    }

    IGC_ASSERT_MESSAGE(pFunc, "%s: getOrInsertFunction probably returned constant expression!",
                       IntrinsicDefinitionT::scFunctionRootName);
    // Since Function::isIntrinsic() will return true due to llvm.* prefix,
    // Module::getOrInsertFunction fails to add the attributes.
    // explicitly adding the attribute to handle this problem.
    // This since is setup on the function declaration, attribute assignment
    // is global and hence this approach suffices.
    pFunc->setAttributes(attribs);

    return pFunc;
  }

  static llvm::FunctionType *GetType(llvm::LLVMContext &ctx, const llvm::ArrayRef<llvm::Type *> &overloadedTypes) {
    constexpr uint8_t numArguments = static_cast<uint8_t>(Argument::Count);
    std::array<llvm::Type *, numArguments + 1> types{};

    TypeResolutionContext resCtx(overloadedTypes);

    // Resolve return type.
    types[0] = IntrinsicDefinitionT::scResTypes.GetType(ctx, resCtx);
    IGC_ASSERT_MESSAGE(types[0] != nullptr, "%s[0]: The type must be defined to determine the function type.",
                       IntrinsicDefinitionT::scFunctionRootName);
    IGC_ASSERT_MESSAGE(IntrinsicDefinitionT::scResTypes.VerifyType(types[0]),
                       "%s[0]: The type is inconsistent with the definition.",
                       IntrinsicDefinitionT::scFunctionRootName);

    // Resolve argument types.
    if constexpr (numArguments > 0) {
      for (uint8_t i = 0; i < numArguments; i++) {
        types[i + 1] = IntrinsicDefinitionT::scArguments[i].m_Type.GetType(ctx, resCtx);
        IGC_ASSERT_MESSAGE(types[i + 1] != nullptr, "%s[%d]: The type must be defined to determine the function type.",
                           IntrinsicDefinitionT::scFunctionRootName, i + 1);
        IGC_ASSERT_MESSAGE(IntrinsicDefinitionT::scArguments[i].m_Type.VerifyType(types[i + 1]),
                           "%s[%d]: The type is inconsistent with the definition.",
                           IntrinsicDefinitionT::scFunctionRootName, i + 1);
      }
    }

#ifndef NDEBUG
    // Phase 2: Cross-argument verification for PropertyRef constraints.
    {
      uint32_t resTypeIdx = 0; // Return type is always at index 0
      VerifyCrossArgConstraints(resTypeIdx, IntrinsicDefinitionT::scResTypes, types[resTypeIdx], overloadedTypes,
                                IntrinsicDefinitionT::scFunctionRootName);
      if constexpr (numArguments > 0) {
        for (uint8_t i = 0; i < numArguments; i++) {
          VerifyCrossArgConstraints(i + 1, IntrinsicDefinitionT::scArguments[i].m_Type, types[i + 1], overloadedTypes,
                                    IntrinsicDefinitionT::scFunctionRootName);
        }
      }
    }
#endif // NDEBUG

    llvm::Type *resultTy = types.front();
    auto argBeg = types.data() + 1;
    bool isVararg = std::next(types.begin()) != types.end() && types.back()->isVoidTy();
    uint32_t numArgs = isVararg ? static_cast<uint32_t>(types.size() - 2) : static_cast<uint32_t>(types.size() - 1);
    llvm::ArrayRef<llvm::Type *> argTys(argBeg, numArgs);
    // Disable this path because of GenISA_UnmaskedRegionBegin and GenISA_UnmaskedRegionEnd
    return llvm::FunctionType::get(resultTy, argTys, false /*isVararg*/);
  }

  static llvm::AttributeList GetAttributeList(llvm::Module &M,
                                              const llvm::ArrayRef<llvm::Type *> &overloadedPointeeTys) {
    auto &ctx = M.getContext();
    // 1. Instantiate regular attributes for the given intrinsic
    llvm::ArrayRef<llvm::Attribute::AttrKind> attributeKinds = IntrinsicDefinitionT::scAttributeKinds;

    auto mainAttrList = llvm::AttributeList::get(ctx, llvm::AttributeList::FunctionIndex, attributeKinds);
    // 2. Gather the memory attribute(s) in a separate routine
    auto memoryAB = IntrinsicDefinitionT::scMemoryEffects.getAsAttrBuilder(ctx);
    mainAttrList = mainAttrList.addFnAttributes(ctx, memoryAB);
    // 3. Gather parameter attributes
    uint8_t overloadedTypeIndex = 0;
    auto RetrieveParamAttr = [&overloadedTypeIndex, &ctx, &overloadedPointeeTys,
                              &mainAttrList](uint8_t index, const ArgumentDescription &arg) {
      if (arg.m_Capture.has_value()) {
        IGCLLVM::addCapture(mainAttrList, ctx, index, arg.m_Capture.value());
      }

      if (arg.m_AttrKind == llvm::Attribute::None) {
        return;
      }

      IGC_ASSERT_MESSAGE(llvm::Attribute::canUseAsParamAttr(arg.m_AttrKind), "Not a param attribute!");

      if (llvm::Attribute::isTypeAttrKind(arg.m_AttrKind)) {
        llvm::Type *pointeeType = nullptr;
        if (overloadedTypeIndex < overloadedPointeeTys.size() && arg.m_Type.IsOverloadable()) {
          pointeeType = overloadedPointeeTys[overloadedTypeIndex++];
        } else {
          pointeeType = arg.m_Type.m_Pointer.m_Type.GetType(ctx);
        }

        // IGC_ASSERT_MESSAGE(pointeeType, "Missing type for the type-dependent attribute!");
        if (!pointeeType)
          return;

        mainAttrList =
            mainAttrList.addParamAttribute(ctx, {index}, llvm::Attribute::get(ctx, arg.m_AttrKind, pointeeType));
      } else {
        mainAttrList = mainAttrList.addParamAttribute(ctx, {index}, llvm::Attribute::get(ctx, arg.m_AttrKind));
      }
    };

    constexpr uint8_t numArguments = static_cast<uint8_t>(Argument::Count);
    if constexpr (numArguments > 0) {
      for (uint8_t i = 0; i < numArguments; i++) {
        RetrieveParamAttr(i, IntrinsicDefinitionT::scArguments[i]);
      }
    }

    return mainAttrList;
  }
};

template <uint32_t... Is> static constexpr auto GetDeclarationFuncArrayImp(std::integer_sequence<uint32_t, Is...>) {
  return std::array{
      &(IntrinsicFunctionImp<static_cast<llvm::GenISAIntrinsic::ID>(Is + scBeginIntrinsicIndex)>::GetDeclaration)...};
}

static constexpr auto GetDeclarationFuncArray() {
  auto seq = std::make_integer_sequence<uint32_t, scNumIntrinsics>();
  return GetDeclarationFuncArrayImp(seq);
}

llvm::Function *GetDeclaration(llvm::Module *pModule, llvm::GenISAIntrinsic::ID id,
                               llvm::ArrayRef<llvm::Type *> overloadedTys,
                               llvm::ArrayRef<llvm::Type *> overloadedPointeeTys) {
  constexpr auto funcArray = GetDeclarationFuncArray();
  llvm::Function *pResult = nullptr;
  uint32_t index = static_cast<uint32_t>(id) - scBeginIntrinsicIndex;
  if (index < funcArray.size()) {
    pResult = funcArray[index](*pModule, overloadedTys, overloadedPointeeTys);
  }
  return pResult;
}

template <uint32_t... Is> static constexpr auto GetNameFuncArrayImp(std::integer_sequence<uint32_t, Is...>) {
  return std::array{
      &(IntrinsicFunctionImp<static_cast<llvm::GenISAIntrinsic::ID>(Is + scBeginIntrinsicIndex)>::GetName)...};
}

static constexpr auto GetNameFuncArray() {
  auto seq = std::make_integer_sequence<uint32_t, scNumIntrinsics>();
  return GetNameFuncArrayImp(seq);
}

std::string GetName(llvm::GenISAIntrinsic::ID id, llvm::ArrayRef<llvm::Type *> overloadedTys,
                    llvm::ArrayRef<llvm::Type *> overloadedPointeeTys) {
  constexpr auto funcArray = GetNameFuncArray();
  std::string result;
  uint32_t index = static_cast<uint32_t>(id) - scBeginIntrinsicIndex;
  if (index < funcArray.size()) {
    result = funcArray[index](overloadedTys, overloadedPointeeTys);
  }
  return result;
}

template <uint32_t... Is> static auto GetIntrinsicCommentsArrayImp(std::integer_sequence<uint32_t, Is...>) {
  return std::array{IntrinsicFunctionImp<static_cast<llvm::GenISAIntrinsic::ID>(
      Is + scBeginIntrinsicIndex)>::GetIntrinsicComments()...};
}

static auto GetIntrinsicCommentsArray() {
  auto seq = std::make_integer_sequence<uint32_t, scNumIntrinsics>();
  return GetIntrinsicCommentsArrayImp(seq);
}

llvm::GenISAIntrinsic::IntrinsicComments GetIntrinsicComments(llvm::GenISAIntrinsic::ID id) {
  static const auto intrinsicCommentsArray = GetIntrinsicCommentsArray();
  uint32_t index = static_cast<uint32_t>(id) - scBeginIntrinsicIndex;
  if (index < intrinsicCommentsArray.size()) {
    return intrinsicCommentsArray[index];
  }
  return {};
}

const char *GetIntrinsicPrefixName() { return scIntrinsicPrefix.data(); }

} // namespace IGC
