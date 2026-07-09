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

} // namespace IGCLLVM

#endif
