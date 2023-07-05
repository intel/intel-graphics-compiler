/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// This file defines a set of enums which allow processing of intrinsic
// functions.  Values of these enum types are returned by
// InternalIntrinsic::getInternalIntrinsicID.
//
//===----------------------------------------------------------------------===//

#ifndef INTERNAL_INTRINSIC_INTERFACE_H
#define INTERNAL_INTRINSIC_INTERFACE_H

#include <llvm/IR/Module.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Intrinsics.h>

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include "Probe/Assertion.h"

namespace vc::InternalIntrinsic {

enum ID : unsigned {
  // TODO: check start, later may be GenXIntrinsic::num_genx_intrinsics
  not_internal_intrinsic = llvm::GenXIntrinsic::not_any_intrinsic + 1,
#define GET_INTRINSIC_ENUM_VALUES
#include "vc/InternalIntrinsics/InternalIntrinsicDescription.gen"
#undef GET_INTRINSIC_ENUM_VALUES
  num_internal_intrinsics,
  not_any_intrinsic,
};

inline const char *getInternalIntrinsicPrefix() { return "llvm.vc.internal."; }

ID getInternalIntrinsicID(const llvm::Function *F);

/// Utility function to get the internal_intrinsic ID if V is a
/// InternalIntrinsic call. V is allowed to be 0.
inline ID getInternalIntrinsicID(const llvm::Value *V) {
  if (!V)
    return InternalIntrinsic::not_any_intrinsic;
  const llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(V);
  if (!CI)
    return InternalIntrinsic::not_any_intrinsic;
  llvm::Function *Callee = CI->getCalledFunction();
  if (!Callee)
    return InternalIntrinsic::not_any_intrinsic;
  return getInternalIntrinsicID(Callee);
}

/// InternalIntrinsic::isInternalIntrinsic(ID) - Is Internal intrinsic
/// NOTE that this is include not_internal_intrinsic
inline bool isInternalIntrinsic(unsigned ID) {
  return ID >= not_internal_intrinsic && ID < num_internal_intrinsics;
}

/// InternalIntrinsic::isInternalIntrinsic(CF) - Returns true if
/// the function's name starts with "llvm.vc.internal.".
/// It's possible for this function to return true while
/// getInternalIntrinsicID() returns InternalIntrinsic::not_internal_intrinsic!
inline bool isInternalIntrinsic(const llvm::Function *CF) {
  IGC_ASSERT(CF);
  return CF->getName().startswith(getInternalIntrinsicPrefix());
}

inline bool isInternalIntrinsic(const llvm::Value *V) {
  if (!V)
    return false;
  const llvm::CallInst *CI = llvm::dyn_cast<llvm::CallInst>(V);
  if (!CI)
    return false;
  llvm::Function *Callee = CI->getCalledFunction();
  if (!Callee)
    return false;
  return isInternalIntrinsic(Callee);
}

/// InternalIntrinsic::isInternalNonTrivialIntrinsic(ID) - Is Internal
/// intrinsic, which is not equal to not_internal_intrinsic or
/// not_internal_intrinsic
inline bool isInternalNonTrivialIntrinsic(unsigned ID) {
  return ID > not_internal_intrinsic && ID < num_internal_intrinsics;
}

inline bool isInternalNonTrivialIntrinsic(const llvm::Function *CF) {
  return isInternalNonTrivialIntrinsic(getInternalIntrinsicID(CF));
}

inline bool isInternalNonTrivialIntrinsic(const llvm::Value *V) {
  return isInternalNonTrivialIntrinsic(getInternalIntrinsicID(V));
}

/// InternalIntrinsic::getInternalDeclaration(M, ID) - Create or insert a
/// Internal LLVM Function declaration for an intrinsic, and return it.
///
/// The Tys parameter is for intrinsics with overloaded types (e.g., those
/// using iAny, fAny, vAny, or iPTRAny).  For a declaration of an overloaded
/// intrinsic, Tys must provide exactly one type for each overloaded type in
/// the intrinsic.
llvm::Function *
getInternalDeclaration(llvm::Module *M, ID id,
                       llvm::ArrayRef<llvm::Type *> Tys = llvm::None);

/// InternalIntrinsic::isOverloadedArg(ID, ArgNum) - Return true if ArgNum
/// in intrinsic overloaded
bool isOverloadedArg(unsigned IntrinID, unsigned ArgNum);

/// InternalIntrinsic::isOverloadedRet(ID) - Return true if return type
/// in intrinsic is overloaded
bool isOverloadedRet(unsigned IntrinID);

std::string getInternalName(InternalIntrinsic::ID id,
                            llvm::ArrayRef<llvm::Type *> Tys);
} // namespace vc::InternalIntrinsic

#endif
