/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DERIVEDTYPES_H
#define IGCLLVM_IR_DERIVEDTYPES_H

#include "Probe/Assertion.h"
#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/Casting.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Module.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include "common/CommonMacros.h"

#include "Type.h"

namespace IGCLLVM {
using FixedVectorType = llvm::FixedVectorType;
const uint32_t VectorTyID = llvm::Type::FixedVectorTyID;
using ShuffleVectorMaskType = int;

inline uint32_t GetVectorTypeBitWidth(llvm::Type *pType) {
  return (uint32_t)pType->getPrimitiveSizeInBits().getFixedValue();
}

inline bool isScalable(const FixedVectorType &Ty) {
  IGC_UNUSED(Ty);
  return false;
}

inline llvm::StructType *getTypeByName(llvm::Module *M, llvm::StringRef Name) {
  return llvm::StructType::getTypeByName(M->getContext(), Name);
}

inline llvm::Type *getWithNewBitWidth(const llvm::Type *Ty, unsigned NewBitWidth) {
  return Ty->getWithNewBitWidth(NewBitWidth);
}

namespace PointerType {

inline llvm::PointerType *get(llvm::PointerType *PT, unsigned AddressSpace) {
#if LLVM_VERSION_MAJOR < 17
  return llvm::PointerType::getWithSamePointeeType(PT, AddressSpace);
#else
  return llvm::PointerType::get(PT->getContext(), AddressSpace);
#endif
}

inline llvm::PointerType *get(llvm::Type *ElementType, unsigned AddressSpace) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::PointerType::get(ElementType, AddressSpace);
#else
  return llvm::PointerType::get(ElementType->getContext(), AddressSpace);
#endif
}

inline llvm::PointerType *get(llvm::LLVMContext &C, unsigned AddressSpace) {
#if LLVM_VERSION_MAJOR < 15
  return llvm::PointerType::get(llvm::Type::getInt8Ty(C), AddressSpace);
#else
  return llvm::PointerType::get(C, AddressSpace);
#endif
}

} // namespace PointerType

inline bool isOpaqueOrPointeeTypeMatches(llvm::PointerType *PT, llvm::Type *Ty) {
#if LLVM_VERSION_MAJOR < 17
  return PT->isOpaqueOrPointeeTypeMatches(Ty);
#else
  return true;
#endif
}

inline bool isOpaque(const llvm::PointerType *PT) {
#if LLVM_VERSION_MAJOR < 17
  return PT->isOpaque();
#else
  return true;
#endif
}

inline bool isOpaquePointerTy(const llvm::Type *Type) {
  if (auto *PTy = llvm::dyn_cast<llvm::PointerType>(Type))
    return isOpaque(PTy);
  return false;
}

inline llvm::PointerType *getInt8PtrTy(llvm::LLVMContext &C, unsigned AS = 0) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::Type::getInt8PtrTy(C, AS);
#else
  return llvm::PointerType::get(C, AS);
#endif
}

inline llvm::PointerType *getInt16PtrTy(llvm::LLVMContext &C, unsigned AS = 0) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::Type::getInt16PtrTy(C, AS);
#else
  return llvm::PointerType::get(C, AS);
#endif
}

inline llvm::PointerType *getInt32PtrTy(llvm::LLVMContext &C, unsigned AS = 0) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::Type::getInt32PtrTy(C, AS);
#else
  return llvm::PointerType::get(C, AS);
#endif
}

inline llvm::PointerType *getInt64PtrTy(llvm::LLVMContext &C, unsigned AS = 0) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::Type::getInt64PtrTy(C, AS);
#else
  return llvm::PointerType::get(C, AS);
#endif
}

inline llvm::PointerType *getIntNPtrTy(llvm::LLVMContext &C, unsigned N, unsigned AS = 0) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::Type::getIntNPtrTy(C, N, AS);
#else
  (void)N;
  return llvm::PointerType::get(C, AS);
#endif
}

} // namespace IGCLLVM

#endif
