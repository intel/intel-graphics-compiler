/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_SCALAREVOLUTION_H
#define IGCLLVM_ANALYSIS_SCALAREVOLUTION_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/ValueTracking.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

inline const llvm::SCEV *getAddExpr(llvm::ScalarEvolution &SE, llvm::SmallVectorImpl<const llvm::SCEV *> &Ops,
                                    llvm::SCEV::NoWrapFlags Flags = llvm::SCEV::FlagAnyWrap, unsigned Depth = 0) {
#if LLVM_VERSION_MAJOR >= 23
  llvm::SmallVector<llvm::SCEVUse> UseOps(Ops.begin(), Ops.end());
  return SE.getAddExpr(UseOps, Flags, Depth);
#else
  return SE.getAddExpr(Ops, Flags, Depth);
#endif
}

inline const llvm::SCEV *getMulExpr(llvm::ScalarEvolution &SE, llvm::SmallVectorImpl<const llvm::SCEV *> &Ops,
                                    llvm::SCEV::NoWrapFlags Flags = llvm::SCEV::FlagAnyWrap, unsigned Depth = 0) {
#if LLVM_VERSION_MAJOR >= 23
  llvm::SmallVector<llvm::SCEVUse> UseOps(Ops.begin(), Ops.end());
  return SE.getMulExpr(UseOps, Flags, Depth);
#else
  return SE.getMulExpr(Ops, Flags, Depth);
#endif
}

inline bool canReuseInstruction(llvm::ScalarEvolution &SE, const llvm::SCEV *S, llvm::Instruction *I,
                                llvm::SmallVectorImpl<llvm::Instruction *> &DropPoisonGeneratingInsts) {
#if LLVM_VERSION_MAJOR >= 18
  return SE.canReuseInstruction(S, I, DropPoisonGeneratingInsts);
#else
  // Reuse only when a poison result would already be UB.
  return llvm::programUndefinedIfPoison(I);
#endif
}

inline void getPoisonGeneratingValues(llvm::ScalarEvolution &SE, llvm::SmallPtrSetImpl<const llvm::Value *> &Result,
                                      const llvm::SCEV *S) {
#if LLVM_VERSION_MAJOR >= 18
  SE.getPoisonGeneratingValues(Result, S);
#else
  // Safe as long as canReuseInstruction above reuses no base that may be poison.
#endif
}

} // namespace IGCLLVM

#endif // IGCLLVM_ANALYSIS_SCALAREVOLUTION_H
