/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICINST_H
#define IGCLLVM_IR_INTRINSICINST_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IntrinsicInst.h>
#include "llvm/IR/DebugInfoMetadata.h"
#include <llvm/Support/Alignment.h>
#include "IGC/common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

namespace IGCLLVM {

/// Returns the pointer argument of a lifetime intrinsic.
/// In LLVM < 22, llvm.lifetime.start/end had the signature (i64 size, ptr p),
/// so the pointer was argument 1. In LLVM >= 22 the size argument was removed
/// and the pointer is now argument 0.
inline llvm::Value *getLifetimeIntrinsicPtr(llvm::IntrinsicInst *II) {
  IGC_ASSERT(II);
  IGC_ASSERT(II->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
             II->getIntrinsicID() == llvm::Intrinsic::lifetime_end);
#if LLVM_VERSION_MAJOR >= 22
  return II->getArgOperand(0);
#else
  return II->getArgOperand(1);
#endif
}

/// Returns the size argument of a lifetime intrinsic, or nullptr on LLVM >= 22
/// where the size argument was removed.
inline llvm::ConstantInt *getLifetimeIntrinsicSize(llvm::IntrinsicInst *II) {
  IGC_ASSERT(II);
  IGC_ASSERT(II->getIntrinsicID() == llvm::Intrinsic::lifetime_start ||
             II->getIntrinsicID() == llvm::Intrinsic::lifetime_end);
#if LLVM_VERSION_MAJOR >= 22
  return nullptr;
#else
  return llvm::cast<llvm::ConstantInt>(II->getArgOperand(0));
#endif
}

/// Masked gather/scatter operand-layout helpers.
///
/// LLVM 22 dropped the explicit i32 alignment operand from
/// llvm.masked.gather / llvm.masked.scatter; the alignment is now an align
/// attribute on the pointer-vector operand. The operand layout changed from:
///   masked_gather (ptrs, i32 align, mask, passthru)
///   masked_scatter(value, ptrs, i32 align, mask)
/// to:
///   masked_gather (ptrs, mask, passthru)
///   masked_scatter(value, ptrs, mask)
/// The gather pointer operand (op0), the scatter pointer operand (op1) and the
/// scatter value operand (op0) keep the same index across versions.

/// Operand index of the mask for llvm.masked.gather.
inline unsigned getMaskedGatherMaskOperandNo() {
#if LLVM_VERSION_MAJOR >= 22
  return 1;
#else
  return 2;
#endif
}

/// Operand index of the passthru for llvm.masked.gather.
inline unsigned getMaskedGatherPassThruOperandNo() {
#if LLVM_VERSION_MAJOR >= 22
  return 2;
#else
  return 3;
#endif
}

/// Operand index of the mask for llvm.masked.scatter.
inline unsigned getMaskedScatterMaskOperandNo() {
#if LLVM_VERSION_MAJOR >= 22
  return 2;
#else
  return 3;
#endif
}

/// Returns the mask (predicate) operand of a masked gather/scatter intrinsic.
inline llvm::Value *getMaskedGatherScatterMask(const llvm::IntrinsicInst *II) {
  IGC_ASSERT(II);
  IGC_ASSERT(II->getIntrinsicID() == llvm::Intrinsic::masked_gather ||
             II->getIntrinsicID() == llvm::Intrinsic::masked_scatter);
  return II->getArgOperand(II->getIntrinsicID() == llvm::Intrinsic::masked_gather ? getMaskedGatherMaskOperandNo()
                                                                                  : getMaskedScatterMaskOperandNo());
}

/// Returns the passthru operand of a masked gather intrinsic.
inline llvm::Value *getMaskedGatherPassThru(const llvm::IntrinsicInst *II) {
  IGC_ASSERT(II);
  IGC_ASSERT(II->getIntrinsicID() == llvm::Intrinsic::masked_gather);
  return II->getArgOperand(getMaskedGatherPassThruOperandNo());
}

/// Returns the alignment of a masked gather/scatter intrinsic.
inline llvm::Align getMaskedGatherScatterAlign(const llvm::IntrinsicInst *II) {
  IGC_ASSERT(II);
  bool IsGather = II->getIntrinsicID() == llvm::Intrinsic::masked_gather;
  IGC_ASSERT(IsGather || II->getIntrinsicID() == llvm::Intrinsic::masked_scatter);
#if LLVM_VERSION_MAJOR >= 22
  return II->getParamAlign(IsGather ? 0 : 1).valueOrOne();
#else
  unsigned AlignOpNo = IsGather ? 1 : 2;
  return llvm::assumeAligned(llvm::cast<llvm::ConstantInt>(II->getArgOperand(AlignOpNo))->getZExtValue());
#endif
}

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

#if LLVM_VERSION_MAJOR >= 22
inline void setExpression(llvm::DbgVariableRecord *DbgRec, llvm::DIExpression *NewExpr) {
  IGC_ASSERT(DbgRec);
  DbgRec->setExpression(NewExpr);
}

inline bool isKillLocation(const llvm::DbgVariableRecord *DbgRec) {
  IGC_ASSERT(DbgRec);
  return DbgRec->isKillLocation();
}

inline llvm::Value *getVariableLocation(const llvm::DbgVariableRecord *DbgRec) {
  IGC_ASSERT(DbgRec);
  IGC_ASSERT_MESSAGE((DbgRec->getNumVariableLocationOps() == 1) || isKillLocation(DbgRec),
                     "unsupported number of location ops");
  return DbgRec->getVariableLocationOp(0);
}

inline void setKillLocation(llvm::DbgVariableRecord *DbgRec) {
  IGC_ASSERT(DbgRec);
  DbgRec->setKillLocation();
}
#endif
} // namespace IGCLLVM

#endif
