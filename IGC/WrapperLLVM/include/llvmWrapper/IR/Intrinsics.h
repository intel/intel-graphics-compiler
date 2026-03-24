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

template <typename... Tys>
inline llvm::Function *getOrInsertDeclaration(llvm::Module *M, llvm::Intrinsic::ID Id, Tys &&...Types) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::Intrinsic::getOrInsertDeclaration(M, Id, std::forward<Tys>(Types)...);
#else
  return llvm::Intrinsic::getDeclaration(M, Id, std::forward<Tys>(Types)...);
#endif
}

} // namespace IGCLLVM

#endif
