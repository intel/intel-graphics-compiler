/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_TARGETTRANSFORMINFO_H
#define IGCLLVM_ANALYSIS_TARGETTRANSFORMINFO_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Support/InstructionCost.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/TargetTransformInfoImpl.h"
#include "llvm/IR/User.h"
#include "llvm/IR/Value.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
template <typename T> class TTIImplCRTPBase : public llvm::TargetTransformInfoImplCRTPBase<T> {
private:
  using CRTPBaseT = llvm::TargetTransformInfoImplCRTPBase<T>;

public:
  TTIImplCRTPBase(const llvm::DataLayout &DL) : CRTPBaseT(DL) {}
  llvm::InstructionCost getInstructionCost(const llvm::User *U, llvm::ArrayRef<const llvm::Value *> Operands,
                                           llvm::TargetTransformInfo::TargetCostKind CostKind) {
#if LLVM_VERSION_MAJOR >= 16
    return CRTPBaseT::getInstructionCost(U, Operands, CostKind);
#else  // LLVM_VERSION_MAJOR
    return CRTPBaseT::getUserCost(U, Operands, CostKind);
#endif // LLVM_VERSION_MAJOR
  }
};
} // namespace IGCLLVM

#endif
