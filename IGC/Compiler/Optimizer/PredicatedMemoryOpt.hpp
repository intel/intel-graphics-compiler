/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Dominators.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Reuse an earlier predicated load when it reads the same value under a weaker
// predicate. A select preserves the later load's fallback value when necessary:
//
//   %a = PredicatedLoad(%ptr, 4, %q1, %fallback1)
//   %b = PredicatedLoad(%ptr, 4, %q2, %fallback2) ; %q2 implies %q1
//   ->
//   %b = select %q2, %a, %fallback2
//
// Both loads must be in the same basic block; the earlier one is then available
// on every lane the later one reads.
class ReusePredicatedLoad : public llvm::FunctionPass {
public:
  static char ID;
  ReusePredicatedLoad();
  llvm::StringRef getPassName() const override { return "ReusePredicatedLoad"; }
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesCFG(); }
  bool runOnFunction(llvm::Function &F) override;
};

// Shrink a load's predicate to the lanes where its result is observable. For
// example, a load used only by the true value of `select %cond` can use the
// smaller predicate `%predicate & %cond`.
//
//   %a = PredicatedLoad(%ptr, 4, %predicate, %fallback)
//   %result = select i1 %cond, i32 %a, i32 %other
//   ->
//   %shrunk = and i1 %predicate, %cond
//   %a = PredicatedLoad(%ptr, 4, %shrunk, %fallback)
//   %result = select i1 %cond, i32 %a, i32 %other
//
// In the case the predicate proves the condition against the use: no lane that
// reads memory is ever observed, the  load is removed in favour of its fallback
// value instead.
class ShrinkLoadPredicate : public llvm::FunctionPass {
public:
  static char ID;
  ShrinkLoadPredicate();
  llvm::StringRef getPassName() const override { return "ShrinkLoadPredicate"; }
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addPreserved<llvm::DominatorTreeWrapperPass>();
  }
  bool runOnFunction(llvm::Function &F) override;
};

llvm::FunctionPass *createReusePredicatedLoadPass();
llvm::FunctionPass *createShrinkLoadPredicatePass();

} // namespace IGC
