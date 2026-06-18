/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_VALUETRACKING_H
#define IGCLLVM_ANALYSIS_VALUETRACKING_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include <llvm/Analysis/ValueTracking.h>
#include <llvm/Support/KnownBits.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::Value *getUnderlyingObject(llvm::Value *V, const llvm::DataLayout &DL) {
  (void)DL;
  return llvm::getUnderlyingObject(V);
}

inline llvm::KnownBits computeKnownBits(const llvm::Value *V, const llvm::DataLayout &DL,
                                        llvm::AssumptionCache *AC = nullptr, const llvm::Instruction *CxtI = nullptr) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::computeKnownBits(V, DL, AC, CxtI);
#else
  return llvm::computeKnownBits(V, DL, 0, AC, CxtI);
#endif
}

inline llvm::KnownBits computeKnownBits(const llvm::Value *V, const llvm::DataLayout &DL, llvm::AssumptionCache *AC,
                                        const llvm::Instruction *CxtI, const llvm::DominatorTree *DT) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::computeKnownBits(V, DL, AC, CxtI, DT);
#else
  return llvm::computeKnownBits(V, DL, 0, AC, CxtI, DT);
#endif
}

inline bool haveNoCommonBitsSet(const llvm::Value *V1, const llvm::Value *V2, const llvm::DataLayout &DL,
                                llvm::AssumptionCache *AC = nullptr, const llvm::Instruction *CxtI = nullptr,
                                const llvm::DominatorTree *DT = nullptr) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::haveNoCommonBitsSet(V1, V2, llvm::SimplifyQuery(DL, DT, AC, CxtI));
#else
  return llvm::haveNoCommonBitsSet(V1, V2, DL, AC, CxtI, DT);
#endif
}
} // namespace IGCLLVM

#endif
