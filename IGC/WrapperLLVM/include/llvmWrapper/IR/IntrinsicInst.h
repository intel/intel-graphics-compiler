/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICINST_H
#define IGCLLVM_IR_INTRINSICINST_H

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IntrinsicInst.h>
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/Support/Casting.h"

#include "Probe/Assertion.h"

namespace IGCLLVM {
inline bool isKillLocation(const llvm::DbgVariableIntrinsic *DbgInst) {
  IGC_ASSERT(DbgInst);
#if LLVM_VERSION_MAJOR <= 15
  return DbgInst->isUndef();
#else // LLVM_VERSION_MAJOR >= 16
  return DbgInst->isKillLocation();
#endif
}

inline llvm::Value *getVariableLocation(const llvm::DbgVariableIntrinsic *DbgInst) {
  IGC_ASSERT(DbgInst);
  IGC_ASSERT_MESSAGE((DbgInst->getNumVariableLocationOps() == 1) || isKillLocation(DbgInst),
                     "unsupported number of location ops");
  return DbgInst->getVariableLocationOp(0);
}

inline void setKillLocation(llvm::DbgVariableIntrinsic *DbgInst) {
  IGC_ASSERT(DbgInst);

#if LLVM_VERSION_MAJOR <= 15
  DbgInst->setUndef();
#else // LLVM_VERSION_MAJOR >= 16
  DbgInst->setKillLocation();
#endif
}

inline void setExpression(llvm::DbgVariableIntrinsic *DbgInst, llvm::DIExpression *NewExpr) {
  IGC_ASSERT(DbgInst);
  DbgInst->setExpression(NewExpr);
}
} // namespace IGCLLVM

#endif
