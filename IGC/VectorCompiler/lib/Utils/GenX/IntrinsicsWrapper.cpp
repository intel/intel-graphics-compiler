/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/IntrinsicsWrapper.h"

using namespace llvm;

unsigned vc::getAnyIntrinsicID(const Function *F) {
  IGC_ASSERT(F);
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(F))
    return GenXIntrinsic::getGenXIntrinsicID(F);
  if (InternalIntrinsic::isInternalNonTrivialIntrinsic(F))
    return InternalIntrinsic::getInternalIntrinsicID(F);

  Intrinsic::ID IID = F->getIntrinsicID();
  // TODO: return the biggest constant from intrinsics ID
  // currently saved the expected behaviour
  // to make sure assert
  // ID != GenXIntrinsics::not_any_intrinsic
  // works right
  if (IID == Intrinsic::not_intrinsic)
    return GenXIntrinsic::not_any_intrinsic;
  return IID;
}

unsigned vc::getAnyIntrinsicID(const llvm::Value *V) {
  if (!V)
    return GenXIntrinsic::not_any_intrinsic;
  const CallInst *CI = dyn_cast<CallInst>(V);
  if (!CI)
    return GenXIntrinsic::not_any_intrinsic;
  Function *Callee = CI->getCalledFunction();
  // TODO: may be this is error?
  // nullptr = this is an indirect function
  // invocation or the function signature
  // does not match the call signature
  if (!Callee)
    return GenXIntrinsic::not_any_intrinsic;

  return getAnyIntrinsicID(Callee);
}

bool vc::isAnyVcIntrinsic(unsigned ID) {
  return InternalIntrinsic::isInternalNonTrivialIntrinsic(ID) ||
         GenXIntrinsic::isGenXNonTrivialIntrinsic(ID);
}

bool vc::isAnyVcIntrinsic(const Function *F) {
  return InternalIntrinsic::isInternalNonTrivialIntrinsic(F) ||
         GenXIntrinsic::isGenXNonTrivialIntrinsic(F);
}

bool vc::isAnyVcIntrinsic(const Value *V) {
  return InternalIntrinsic::isInternalNonTrivialIntrinsic(V) ||
         GenXIntrinsic::isGenXNonTrivialIntrinsic(V);
}

bool vc::isAnyNonTrivialIntrinsic(unsigned ID) {
  return InternalIntrinsic::isInternalNonTrivialIntrinsic(ID) ||
         GenXIntrinsic::isAnyNonTrivialIntrinsic(ID);
}

bool vc::isOverloadedRet(unsigned ID) {
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(ID))
    return GenXIntrinsic::isOverloadedRet(ID);
  if (vc::InternalIntrinsic::isInternalNonTrivialIntrinsic(ID))
    return vc::InternalIntrinsic::isOverloadedRet(ID);
  // Assume that LLVM intrinsics can only be overloaded by the return value
  // type.
  // FIXME: This is not true for all LLVM intrinsics.
  return Intrinsic::isOverloaded(ID);
}

bool vc::isOverloadedArg(unsigned ID, unsigned ArgumentNum) {
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(ID))
    return GenXIntrinsic::isOverloadedArg(ID, ArgumentNum);
  if (vc::InternalIntrinsic::isInternalNonTrivialIntrinsic(ID))
    return vc::InternalIntrinsic::isOverloadedArg(ID, ArgumentNum);
  return false;
}

Function *vc::getAnyDeclaration(Module *M, unsigned ID, ArrayRef<Type *> Tys) {
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(ID))
    return GenXIntrinsic::getGenXDeclaration(
        M, static_cast<llvm::GenXIntrinsic::ID>(ID), Tys);
  if (InternalIntrinsic::isInternalNonTrivialIntrinsic(ID))
    return InternalIntrinsic::getInternalDeclaration(
        M, static_cast<vc::InternalIntrinsic::ID>(ID), Tys);
  return Intrinsic::getDeclaration(M, static_cast<Intrinsic::ID>(ID), Tys);
}

std::string vc::getAnyName(unsigned Id, ArrayRef<Type *> Tys) {
  if (vc::InternalIntrinsic::isInternalIntrinsic(Id))
    return vc::InternalIntrinsic::getInternalName(
        static_cast<InternalIntrinsic::ID>(Id), Tys);
  return GenXIntrinsic::getAnyName(Id, Tys);
}

llvm::Function *
vc::getAnyDeclarationForArgs(llvm::Module *M, unsigned ID, Type *RetTy,
                             llvm::ArrayRef<llvm::Value *> Args) {
  SmallVector<Type *, 8> Tys;

  if (vc::isOverloadedRet(ID))
    Tys.push_back(RetTy);

  for (auto &ArgIdx : llvm::enumerate(Args))
    if (vc::isOverloadedArg(ID, ArgIdx.index()))
      Tys.push_back(ArgIdx.value()->getType());

  return vc::getAnyDeclaration(M, ID, Tys);
}

bool vc::isAbsIntrinsic(unsigned ID) {
  if (ID == Intrinsic::fabs)
    return true;
  return GenXIntrinsic::isAbs(ID);
}
