/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2025 Intel Corporation

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
#include <optional>

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
                       llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type*>());

/// InternalIntrinsic::isOverloadedArg(ID, ArgNum) - Return true if ArgNum
/// in intrinsic overloaded
bool isOverloadedArg(unsigned IntrinID, unsigned ArgNum);

/// InternalIntrinsic::isOverloadedRet(ID) - Return true if return type
/// in intrinsic is overloaded
bool isOverloadedRet(unsigned IntrinID);

std::string getInternalName(ID id,
                            llvm::ArrayRef<llvm::Type *> Tys = llvm::ArrayRef<llvm::Type*>());

bool isInternalMemoryIntrinsic(ID id);

inline bool isInternalMemoryIntrinsic(const llvm::Value *V) {
  return isInternalMemoryIntrinsic(getInternalIntrinsicID(V));
}

inline bool isInternalMemoryIntrinsic(const llvm::Function *F) {
  return isInternalMemoryIntrinsic(getInternalIntrinsicID(F));
}

bool isStatelessIntrinsic(ID IID);

inline bool isStatelessIntrinsic(const llvm::Value *V) {
  return isStatelessIntrinsic(getInternalIntrinsicID(V));
}

inline bool isStatelessIntrinsic(const llvm::Function *F) {
  return isStatelessIntrinsic(getInternalIntrinsicID(F));
}

bool isSlmIntrinsic(ID IID);

inline bool isSlmIntrinsic(const llvm::Value *V) {
  return isSlmIntrinsic(getInternalIntrinsicID(V));
}

inline bool isSlmIntrinsic(const llvm::Function *F) {
  return isSlmIntrinsic(getInternalIntrinsicID(F));
}

bool isInternalSamplerIntrinsic(ID IID);

inline bool isInternalSamplerIntrinsic(const llvm::Value *V) {
  return isInternalSamplerIntrinsic(getInternalIntrinsicID(V));
}

inline bool isInternalSamplerIntrinsic(const llvm::Function *F) {
  return isInternalSamplerIntrinsic(getInternalIntrinsicID(F));
}

bool isUntypedBlockLoad2dIntrinsic(ID IID);

bool isMemoryBlockIntrinsic(const llvm::Instruction *I);

unsigned getMemoryVectorSizePerLane(const llvm::Instruction *I);
unsigned getMemorySimdWidth(const llvm::Instruction *I);
unsigned getMemoryRegisterElementSize(const llvm::Instruction *I);

int getMemorySurfaceOperandIndex(unsigned IID);
inline int getMemorySurfaceOperandIndex(const llvm::Instruction *I) {
  return getMemorySurfaceOperandIndex(getInternalIntrinsicID(I));
}

int getMemorySamplerOperandIndex(unsigned IID);

int getMemoryCacheControlOperandIndex(unsigned IID);

inline int getMemoryCacheControlOperandIndex(const llvm::Value *V) {
  return getMemoryCacheControlOperandIndex(getInternalIntrinsicID(V));
}

inline llvm::Value *getMemoryCacheControlOperand(const llvm::Instruction *I) {
  const auto Index = getMemoryCacheControlOperandIndex(I);
  if (Index < 0)
    return nullptr;
  return I->getOperand(Index);
}

int getMemoryAddressOperandIndex(unsigned IID);
inline int getMemoryAddressOperandIndex(const llvm::Instruction *I) {
  return getMemoryAddressOperandIndex(getInternalIntrinsicID(I));
}

inline llvm::Value *getMemoryAddressOperand(const llvm::Instruction *I) {
  const auto Index = getMemoryAddressOperandIndex(I);
  if (Index < 0)
    return nullptr;
  return I->getOperand(Index);
}

int getMemoryBaseOperandIndex(unsigned IID);

inline llvm::Value *getMemoryBaseOperand(const llvm::Instruction *I) {
  auto IID = getInternalIntrinsicID(I);
  const auto Index = getMemoryBaseOperandIndex(IID);
  if (Index < 0)
    return nullptr;
  return I->getOperand(Index);
}

int getTwoAddrOpIndex(const llvm::CallInst *CI);

} // namespace vc::InternalIntrinsic

#endif
