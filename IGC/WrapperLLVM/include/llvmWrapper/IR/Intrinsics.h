/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICS_H
#define IGCLLVM_IR_INTRINSICS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Intrinsics.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include <utility>

namespace IGCLLVM {
using Intrinsic = llvm::Intrinsic::IndependentIntrinsics;

inline llvm::Function *getOrInsertDeclaration(llvm::Module *M, llvm::Intrinsic::ID Id,
                                              llvm::ArrayRef<llvm::Type *> Tys = {}) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::Intrinsic::getOrInsertDeclaration(M, Id, Tys);
#else
  return llvm::Intrinsic::getDeclaration(M, Id, Tys);
#endif
}

inline bool isSignatureValid(llvm::Intrinsic::ID ID, llvm::FunctionType *FTy,
                             llvm::ArrayRef<llvm::Intrinsic::IITDescriptor> &Infos,
                             llvm::SmallVectorImpl<llvm::Type *> &OverloadTys) {
#if LLVM_VERSION_MAJOR >= 23
  return llvm::Intrinsic::isSignatureValid(ID, FTy, OverloadTys);
#else
  return llvm::Intrinsic::matchIntrinsicSignature(FTy, Infos, OverloadTys) ==
         llvm::Intrinsic::MatchIntrinsicTypes_Match;
#endif
}

inline unsigned getOverloadIndex(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.getOverloadIndex();
#else
  return D.getArgumentNumber();
#endif
}

inline unsigned getOneNthEltsVecRefIndex(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.getOverloadIndex();
#else
  return D.getRefArgNumber();
#endif
}

inline llvm::ElementCount getVectorWidth(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.VectorWidth;
#else
  return D.Vector_Width;
#endif
}

inline unsigned getIntegerWidth(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.IntegerWidth;
#else
  return D.Integer_Width;
#endif
}

inline unsigned getPointerAddressSpace(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.PointerAddressSpace;
#else
  return D.Pointer_AddressSpace;
#endif
}

inline unsigned getStructNumElements(const llvm::Intrinsic::IITDescriptor &D) {
#if LLVM_VERSION_MAJOR >= 23
  return D.StructNumElements;
#else
  return D.Struct_NumElements;
#endif
}

namespace IITDescriptorKind {
using KindTy = llvm::Intrinsic::IITDescriptor::IITDescriptorKind;

#if LLVM_VERSION_MAJOR >= 23
inline constexpr KindTy Overloaded = llvm::Intrinsic::IITDescriptor::Overloaded;
inline constexpr KindTy Extend = llvm::Intrinsic::IITDescriptor::Extend;
inline constexpr KindTy Trunc = llvm::Intrinsic::IITDescriptor::Trunc;
inline constexpr KindTy SameVecWidth = llvm::Intrinsic::IITDescriptor::SameVecWidth;
#else
inline constexpr KindTy Overloaded = llvm::Intrinsic::IITDescriptor::Argument;
inline constexpr KindTy Extend = llvm::Intrinsic::IITDescriptor::ExtendArgument;
inline constexpr KindTy Trunc = llvm::Intrinsic::IITDescriptor::TruncArgument;
inline constexpr KindTy SameVecWidth = llvm::Intrinsic::IITDescriptor::SameVecWidthArgument;
#endif

#if LLVM_VERSION_MAJOR >= 23
inline constexpr KindTy OneNthEltsVec = llvm::Intrinsic::IITDescriptor::OneNthEltsVec;
#elif LLVM_VERSION_MAJOR >= 22
inline constexpr KindTy OneNthEltsVec = llvm::Intrinsic::IITDescriptor::OneNthEltsVecArgument;
#endif
} // namespace IITDescriptorKind

} // namespace IGCLLVM

#endif
