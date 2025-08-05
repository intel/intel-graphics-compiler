/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenIntrinsicFunctions.h"
#include "GenIntrinsicDefinition.h"
#include "GenIntrinsicLookup.h"
#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/IR/Module.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/ADT/StringExtras.h>
#include <llvm/CodeGen/ValueTypes.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

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
    if (!IGCLLVM::isOpaquePointerTy(PTyp)) {
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
    llvm::Function *pFunc = llvm::cast<llvm::Function>(M.getOrInsertFunction(funcName, pFuncType, attribs));

    IGC_ASSERT_MESSAGE(pFunc, "getOrInsertFunction probably returned constant expression!");
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

    uint8_t overloadedTypeIndex = 0;
    auto RetrieveType = [&overloadedTypeIndex, &ctx, &types, &overloadedTypes](uint8_t index,
                                                                               const TypeDescription &typeDef) {
      llvm::Type *&pDest = types[index];
      switch (typeDef.m_ID) {
      case TypeID::ArgumentReference: {
        uint8_t argIndex = typeDef.m_Reference.m_Index;
        IGC_ASSERT_MESSAGE(argIndex < overloadedTypes.size(),
                           "Argument reference index must point out one of the overloaded types");
        pDest = overloadedTypes[argIndex];
        break;
      }
      default:
        if (overloadedTypeIndex < overloadedTypes.size() && typeDef.IsOverloadable()) {
          pDest = overloadedTypes[overloadedTypeIndex++];
        } else {
          pDest = typeDef.GetType(ctx);
        }
        break;
      }
      IGC_ASSERT_MESSAGE(pDest != nullptr, "The type must be defined to determine the function type.");
      // IGC_ASSERT_MESSAGE(typeDef.VerifyType(pDest), "The type is inconsistent with the definition.");
    };

    constexpr uint8_t resTypeIndex = 0;
    RetrieveType(resTypeIndex, IntrinsicDefinitionT::scResTypes);

    if constexpr (numArguments > 0) {
      for (uint8_t i = 0; i < numArguments; i++) {
        RetrieveType(i + 1, IntrinsicDefinitionT::scArguments[i].m_Type);
      }
    }
    // IGC_ASSERT(overloadedTypeIndex == overloadedTypes.size());

    llvm::Type **pBegin = types.data() + 1;
    size_t size = types.size() - 1;
    llvm::Type *resultTy = types[0];
    llvm::SmallVector<llvm::Type *, 8> argTys(pBegin, pBegin + size);
    if (!argTys.empty() && argTys.back()->isVoidTy()) {
      argTys.pop_back();
      // Disable this path because of GenISA_UnmaskedRegionBegin and GenISA_UnmaskedRegionEnd
      // return llvm::FunctionType::get(resultTy, argTys, true);
    }
    return llvm::FunctionType::get(resultTy, argTys, false);
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
