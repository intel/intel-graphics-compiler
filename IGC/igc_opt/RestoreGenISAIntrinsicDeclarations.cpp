/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RestoreGenISAIntrinsicDeclarations.h"

#include "GenISAIntrinsics/GenIntrinsics.h"
#include "GenIntrinsicDefinition.h"
#include "llvmWrapper/IR/Argument.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/raw_ostream.h"
#include "common/LLVMWarningsPop.hpp"

#include <array>
#include <utility>

using namespace llvm;

namespace {

constexpr uint32_t BeginIntrinsicIndex = static_cast<uint32_t>(GenISAIntrinsic::ID::no_intrinsic) + 1;
constexpr uint32_t NumIntrinsics =
    static_cast<uint32_t>(GenISAIntrinsic::ID::num_genisa_intrinsics) - BeginIntrinsicIndex;

struct OverloadInfo {
  SmallVector<Type *, 8> OverloadedTys;
  SmallVector<Type *, 4> PointeeTys;
  bool AllPointeeTysInferred = true;
};

static Type *inferPointeeType(const Function &F, unsigned ArgIndex, const IGC::ArgumentDescription &ArgDesc) {
  if (ArgIndex >= F.arg_size())
    return nullptr;

  const Argument *Arg = F.getArg(ArgIndex);

  if (Type *AttrType = IGCLLVM::getArgAttrEltTy(Arg))
    return AttrType;

  Type *ArgType = Arg->getType();
  if (auto *PtrType = dyn_cast<PointerType>(ArgType); PtrType && !IGCLLVM::isOpaque(PtrType))
    return IGCLLVM::getNonOpaquePtrEltTy(PtrType);

  if (ArgDesc.m_Type.m_ID == IGC::TypeID::Pointer)
    return ArgDesc.m_Type.m_Pointer.m_Type.GetType(F.getContext());

  return nullptr;
}

template <GenISAIntrinsic::ID Id> static OverloadInfo inferOverloadInfo(const Function &F) {
  using IntrinsicDefinitionT = IGC::IntrinsicDefinition<Id>;
  using Argument = typename IntrinsicDefinitionT::Argument;

  OverloadInfo Info;
  constexpr unsigned NumArguments = static_cast<unsigned>(Argument::Count);

  auto CollectIfOverloaded = [&Info](const IGC::TypeDescription &TypeDesc, Type *ActualType) {
    if (TypeDesc.m_ID != IGC::TypeID::ArgumentReference && TypeDesc.IsOverloadable())
      Info.OverloadedTys.push_back(ActualType);
  };

  CollectIfOverloaded(IntrinsicDefinitionT::scResTypes, F.getReturnType());

  if constexpr (NumArguments > 0) {
    for (unsigned I = 0; I < NumArguments && I < F.arg_size(); ++I) {
      const IGC::ArgumentDescription &ArgDesc = IntrinsicDefinitionT::scArguments[I];
      CollectIfOverloaded(ArgDesc.m_Type, F.getArg(I)->getType());

      if (!Attribute::isTypeAttrKind(ArgDesc.m_AttrKind) || !ArgDesc.m_Type.IsOverloadable())
        continue;

      if (Info.AllPointeeTysInferred) {
        Type *PointeeType = inferPointeeType(F, I, ArgDesc);
        if (PointeeType) {
          Info.PointeeTys.push_back(PointeeType);
        } else {
          Info.PointeeTys.clear();
          Info.AllPointeeTysInferred = false;
        }
      }
    }
  }

  return Info;
}

// Look up the canonical declaration for the given intrinsic, handling
// name collisions
static Function *getCanonicalDeclaration(Function &F, GenISAIntrinsic::ID Id, const OverloadInfo &Info) {
  ArrayRef<Type *> PointeeTys = Info.AllPointeeTysInferred ? ArrayRef<Type *>(Info.PointeeTys) : ArrayRef<Type *>{};

  Function *Decl = GenISAIntrinsic::getDeclaration(F.getParent(), Id, Info.OverloadedTys, PointeeTys);

  // Module::getOrInsertFunction may return an existing
  // function whose canonical name matches but has a different FunctionType
  // (e.g. a misnamed intrinsic occupying the target name slot with a different
  // address space). Rename the conflicting function and retry — it will be
  // restored to its own correct name when processed later.
  if (Decl != &F && Decl->getFunctionType() != F.getFunctionType()) {
    Decl->setName(Decl->getName() + ".tmp");
    Decl = GenISAIntrinsic::getDeclaration(F.getParent(), Id, Info.OverloadedTys, PointeeTys);
  }

  return Decl;
}

template <GenISAIntrinsic::ID Id> static bool restoreIntrinsicDeclaration(Function &F) {
  using IntrinsicDefinitionT = IGC::IntrinsicDefinition<Id>;
  using Argument = typename IntrinsicDefinitionT::Argument;
  constexpr unsigned NumArguments = static_cast<unsigned>(Argument::Count);

  // getDeclaration strips a trailing void argument (used as a no-arg sentinel
  // in the YAML), so account for that when comparing against the actual
  // function signature.
  unsigned EffectiveNumArguments = NumArguments;
  if constexpr (NumArguments > 0) {
    if (IntrinsicDefinitionT::scArguments[NumArguments - 1].m_Type.m_ID == IGC::TypeID::Void)
      EffectiveNumArguments -= 1;
  }

  if (F.arg_size() != EffectiveNumArguments) {
    errs() << "GenISA intrinsic '" << F.getName() << "' operand number mismatch: expected " << EffectiveNumArguments
           << ", got " << F.arg_size() << "\n";
    IGC_ASSERT_EXIT_MESSAGE(false, "GenISA intrinsic operand number mismatch");
  }

  // Verify return type when the definition specifies a concrete (non-overloaded) type.
  // GetType(ctx) without TypeResolutionContext asserts for overloaded types.
  if (!IntrinsicDefinitionT::scResTypes.IsOverloadable()) {
    if (Type *ExpectedRetTy = IntrinsicDefinitionT::scResTypes.GetType(F.getContext())) {
      if (F.getReturnType() != ExpectedRetTy) {
        errs() << "GenISA intrinsic '" << F.getName() << "' return type mismatch: expected " << *ExpectedRetTy
               << ", got " << *F.getReturnType() << "\n";
        IGC_ASSERT_EXIT_MESSAGE(false, "GenISA intrinsic return type mismatch");
      }
    }
  }

  // Verify non-overloaded argument types match the definition.
  // Skip overloadable types — they can't be resolved without context.
  if constexpr (NumArguments > 0) {
    for (unsigned I = 0; I < EffectiveNumArguments; ++I) {
      const IGC::TypeDescription &ArgTypeDef = IntrinsicDefinitionT::scArguments[I].m_Type;
      if (ArgTypeDef.IsOverloadable())
        continue;
      Type *ExpectedArgTy = ArgTypeDef.GetType(F.getContext());
      if (!ExpectedArgTy)
        continue;
      if (F.getArg(I)->getType() != ExpectedArgTy) {
        errs() << "GenISA intrinsic '" << F.getName() << "' argument " << I << " type mismatch: expected "
               << *ExpectedArgTy << ", got " << *F.getArg(I)->getType() << "\n";
        IGC_ASSERT_EXIT_MESSAGE(false, "GenISA intrinsic argument type mismatch");
      }
    }
  }

  OverloadInfo Info = inferOverloadInfo<Id>(F);

  AttributeList OldAttrs = F.getAttributes();
  std::string OldName = F.getName().str();

  Function *CanonicalDecl = getCanonicalDeclaration(F, Id, Info);

  bool Changed = CanonicalDecl->getAttributes() != OldAttrs || CanonicalDecl->getName() != OldName;
  if (CanonicalDecl == &F)
    return Changed;

  IGC_ASSERT_EXIT_MESSAGE(CanonicalDecl->getFunctionType() == F.getFunctionType(),
                          "Canonical GenISA declaration type mismatch");

  CanonicalDecl->setCallingConv(F.getCallingConv());
  if (!F.use_empty())
    F.replaceAllUsesWith(CanonicalDecl);

  F.eraseFromParent();
  return true;
}

template <uint32_t... Is>
static constexpr auto getRestoreDeclarationFuncArrayImpl(std::integer_sequence<uint32_t, Is...>) {
  return std::array{&(restoreIntrinsicDeclaration<static_cast<GenISAIntrinsic::ID>(Is + BeginIntrinsicIndex)>)...};
}

static constexpr auto getRestoreDeclarationFuncArray() {
  return getRestoreDeclarationFuncArrayImpl(std::make_integer_sequence<uint32_t, NumIntrinsics>());
}

} // namespace

bool restoreGenISAIntrinsicDeclarations(Module &M) {
  static constexpr auto RestoreDeclarationFuncs = getRestoreDeclarationFuncArray();

  // Store intrinsic index alongside the function pointer so we don't depend on
  // getIntrinsicID during processing — a function may be renamed by collision
  // handling in a prior iteration.
  SmallVector<std::pair<Function *, uint32_t>, 32> GenIntrinsics;
  for (Function &F : M) {
    if (!F.isDeclaration() || !GenISAIntrinsic::isIntrinsic(&F))
      continue;

    GenISAIntrinsic::ID Id = GenISAIntrinsic::getIntrinsicID(&F);
    if (Id != GenISAIntrinsic::ID::no_intrinsic) {
      uint32_t Index = static_cast<uint32_t>(Id) - BeginIntrinsicIndex;
      GenIntrinsics.push_back({&F, Index});
    }
  }

  bool Changed = false;
  for (auto [F, Index] : GenIntrinsics) {
    if (Index < RestoreDeclarationFuncs.size())
      Changed |= RestoreDeclarationFuncs[Index](*F);
  }

  return Changed;
}
