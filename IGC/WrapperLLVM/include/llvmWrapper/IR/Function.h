/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_FUNCTION_H
#define IGCLLVM_IR_FUNCTION_H

#include "llvm/IR/Function.h"

#include "Probe/Assertion.h"

namespace IGCLLVM {

inline llvm::Argument *getArg(const llvm::Function &F, unsigned ArgNo) {
  IGC_ASSERT(F.arg_size() > ArgNo);
  llvm::Argument *Arg = nullptr;

#if LLVM_VERSION_MAJOR < 10
  // similar to lvm::Function::getArg implementation
  auto ArgIt = F.arg_begin();
  std::advance(ArgIt, ArgNo);
  Arg = const_cast<llvm::Argument *>(&*ArgIt);
#else
  Arg = F.getArg(ArgNo);
#endif

  return Arg;
}

} // namespace IGCLLVM

#endif
