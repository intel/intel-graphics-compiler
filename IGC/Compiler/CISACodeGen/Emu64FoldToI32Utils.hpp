/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Shared helpers for folding i64 emulated operations to i32 when the operands
// are known to fit in 32 bits.

#ifndef EMU64FOLDTOI32UTILS_H
#define EMU64FOLDTOI32UTILS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DataLayout.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Value.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/Support/KnownBits.h"
#include "common/LLVMWarningsPop.hpp"
#include "igc_regkeys.hpp"

inline bool isHiPartKnownZero(llvm::Value *Hi, const llvm::DataLayout &DL) {
  if (IGC_IS_FLAG_DISABLED(EnableEmuFolding)) {
    return false;
  }
  if (auto *C = llvm::dyn_cast<llvm::ConstantInt>(Hi)) {
    return C->isZero();
  }
  if (IGC_IS_FLAG_ENABLED(EnableAggresiveEmuFolding)) {
    llvm::KnownBits KB = llvm::computeKnownBits(Hi, DL);
    return KB.isZero();
  }
  return false;
}

inline unsigned getActiveBits(llvm::Value *V, const llvm::DataLayout &DL) {
  llvm::KnownBits KB = llvm::computeKnownBits(V, DL);
  return KB.countMaxActiveBits();
}

// For unsigned addition: if both operands have at most N active bits, the result has at most N+1 active bits.
inline bool canFoldAddToI32(llvm::Value *L0, llvm::Value *H0, llvm::Value *L1, llvm::Value *H1,
                            const llvm::DataLayout &DL) {
  if (!isHiPartKnownZero(H0, DL) || !isHiPartKnownZero(H1, DL)) {
    return false;
  }

  unsigned Bits0 = getActiveBits(L0, DL);
  unsigned Bits1 = getActiveBits(L1, DL);
  return (std::max(Bits0, Bits1) + 1) <= 32;
}

// If subtraction has nuw result is non-negative and fits in 32 bits.
inline bool canFoldSubToI32(llvm::BinaryOperator &BinOp, llvm::Value *H0, llvm::Value *H1, const llvm::DataLayout &DL) {
  if (!isHiPartKnownZero(H0, DL) || !isHiPartKnownZero(H1, DL)) {
    return false;
  }
  return BinOp.hasNoUnsignedWrap();
}

// For unsigned multiplication: result has at most Bits0 + Bits1 active bits.
inline bool canFoldMulToI32(llvm::Value *L0, llvm::Value *H0, llvm::Value *L1, llvm::Value *H1,
                            const llvm::DataLayout &DL) {
  if (!isHiPartKnownZero(H0, DL) || !isHiPartKnownZero(H1, DL)) {
    return false;
  }
  unsigned Bits0 = getActiveBits(L0, DL);
  unsigned Bits1 = getActiveBits(L1, DL);
  return (Bits0 + Bits1) <= 32;
}

#endif // EMU64FOLDTOI32UTILS_H
