/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instruction.h"
#if LLVM_VERSION_MAJOR >= 22
#include "llvm/IR/DebugProgramInstruction.h"
#else
#include "llvm/IR/IntrinsicInst.h"
#endif
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/IntrinsicInst.h"

namespace IGC {

// DbgVarInstEntry: the object that carries debug variable location info.
//   LLVM <  22: DbgVariableIntrinsic — an Instruction in the instruction stream.
//   LLVM >= 22: DbgVariableRecord   — a non-instruction debug record attached
//               to a real instruction via getInstruction().
#if LLVM_VERSION_MAJOR >= 22
using DbgVarInstEntry = llvm::DbgVariableRecord;
#else
using DbgVarInstEntry = llvm::DbgVariableIntrinsic;
#endif

// Typed map key for m_DbgVarStorageMap and getStorageOffset/Size.
// Prefer this over void* for debuggability.
using DbgVarStorageKey = const DbgVarInstEntry *;

// API-compatibility helpers — use these in place of per-function #if guards.

// Iterate over all debug variable records/intrinsics attached to instruction I,
// calling Fn(DbgVarInstEntry*) for each one.
template <typename Fn> inline void forEachDbgVar(llvm::Instruction &I, Fn &&callback) {
#if LLVM_VERSION_MAJOR >= 22
  for (llvm::DbgRecord &DR : I.getDbgRecordRange())
    if (auto *DVR = llvm::dyn_cast<llvm::DbgVariableRecord>(&DR))
      callback(DVR);
#else
  if (auto *DVI = llvm::dyn_cast<llvm::DbgVariableIntrinsic>(&I))
    callback(DVI);
#endif
}

// Set kill location on a debug variable entry.
inline void dbgVarSetKillLocation(DbgVarInstEntry *E) { IGCLLVM::setKillLocation(E); }

// Returns the primary value operand of a debug variable entry.
// Both DbgVariableIntrinsic and DbgVariableRecord expose getVariableLocationOp(0)
// as the underlying value accessor, so no version guard is needed.
inline llvm::Value *dbgVarGetValue(const DbgVarInstEntry *E) { return E->getVariableLocationOp(0); }

inline bool dbgVarIsDecl(const DbgVarInstEntry *E) {
#if LLVM_VERSION_MAJOR >= 22
  return E->isDbgDeclare();
#else
  return llvm::isa<llvm::DbgDeclareInst>(E);
#endif
}

inline bool dbgVarIsValue(const DbgVarInstEntry *E) {
#if LLVM_VERSION_MAJOR >= 22
  return E->isDbgValue();
#else
  return llvm::isa<llvm::DbgValueInst>(E);
#endif
}

// Returns the anchor instruction for label/range tracking:
//   DbgVariableRecord    -> the instruction it is attached to
//   DbgVariableIntrinsic -> the instruction itself
inline const llvm::Instruction *dbgVarAnchorInst(const DbgVarInstEntry *E) {
#if LLVM_VERSION_MAJOR >= 22
  return E->getInstruction();
#else
  return E;
#endif
}

// Returns true if two debug variable entries have identical/equivalent content.
// DbgVariableRecord (>= 22) exposes isEquivalentTo(); DbgVariableIntrinsic (< 22)
// inherits Instruction::isIdenticalTo().
inline bool dbgVarIsEquivalentTo(const DbgVarInstEntry *A, const DbgVarInstEntry *B) {
#if LLVM_VERSION_MAJOR >= 22
  return A->isEquivalentTo(*B);
#else
  return A->isIdenticalTo(B);
#endif
}

} // namespace IGC
