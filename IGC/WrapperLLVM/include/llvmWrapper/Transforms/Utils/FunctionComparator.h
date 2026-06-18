/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_FUNCTIONCOMPARATOR_H
#define IGCLLVM_TRANSFORMS_UTILS_FUNCTIONCOMPARATOR_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Utils/FunctionComparator.h"
#if LLVM_VERSION_MAJOR >= 22
#include "llvm/IR/StructuralHash.h"
#endif
#include "common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

#if LLVM_VERSION_MAJOR >= 22
using FunctionHashTy = llvm::stable_hash;
static inline FunctionHashTy computeFunctionHash(const llvm::Function &F) { return llvm::StructuralHash(F); }
#else
using FunctionHashTy = llvm::FunctionComparator::FunctionHash;
static inline FunctionHashTy computeFunctionHash(const llvm::Function &F) {
  return llvm::FunctionComparator::functionHash(const_cast<llvm::Function &>(F));
}
#endif

} // namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_UTILS_FUNCTIONCOMPARATOR_H
