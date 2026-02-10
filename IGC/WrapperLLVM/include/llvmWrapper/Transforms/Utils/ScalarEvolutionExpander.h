/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_SCALAREVOLUTIONEXPANDER_H
#define IGCLLVM_TRANSFORMS_UTILS_SCALAREVOLUTIONEXPANDER_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/Transforms/Utils/ScalarEvolutionExpander.h>
#include "common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

namespace IGCLLVM {
bool isSafeToExpand(const llvm::SCEV *S, llvm::ScalarEvolution *SE, llvm::SCEVExpander *SCEVE) {
  IGC_ASSERT(SE);
  IGC_ASSERT(SCEVE);
#if (LLVM_VERSION_MAJOR < 15)
  return isSafeToExpand(S, *SE);
#else
  return SCEVE->isSafeToExpand(S);
#endif
}

bool isSafeToExpandAt(const llvm::SCEV *S, const llvm::Instruction *InsertionPoint, llvm::ScalarEvolution *SE,
                      llvm::SCEVExpander *SCEVE) {
  IGC_ASSERT(SE);
  IGC_ASSERT(SCEVE);
#if (LLVM_VERSION_MAJOR < 15)
  return isSafeToExpandAt(S, InsertionPoint, *SE);
#else
  return SCEVE->isSafeToExpandAt(S, InsertionPoint);
#endif
}
} // namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_UTILS_SCALAREVOLUTIONEXPANDER_H
