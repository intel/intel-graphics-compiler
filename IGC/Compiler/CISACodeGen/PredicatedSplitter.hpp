/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/SoAAcrossFNUtil.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/DenseSet.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class DataLayout;
class DominatorTree;
class PHINode;
class Value;
} // namespace llvm

namespace IGC {

// Replace loads through the merge (plain loads or existing predicated loads) with predicated loads at the same
// location. Incomings are grouped by their base pointers and for each group pointer is rebuilt at the load from values
// that dominate the merge block (any other value is carried to the merge block by a value-level PHI, which is 0 for the
// other groups), and the load becomes one predicated load per group, gated by an i1 PHI that says which group the merge
// came from.
//
//   merge: %isA = phi i1 [ true, bbA ], [ false, bbB ]    %isB = phi i1 [ false, bbA ], [ true, bbB ]
//          %idxA = phi i64 [ %idx, bbA ], [ 0, bbB ]
//   use:   %a = predicated.load(gep %stack, %idxA, %isA)  %b = predicated.load(%node.field, %isB)
//          %v = select %isA, %a, %b
class PredicatedSplitter {
public:
  PredicatedSplitter(llvm::PHINode *Phi, llvm::DominatorTree &DT, const llvm::DataLayout &DL,
                     const llvm::DenseSet<const llvm::Value *> &ExtraRoots)
      : Phi(Phi), DT(DT), DL(DL), ExtraRoots(ExtraRoots) {}

  // Whether every incoming pointer can be rebuilt at the loads.
  bool canSplit() const;

  // Replaces the loads by predicated loads; the original loads are left without uses.
  void split(llvm::ArrayRef<SoAAcrossFNUtil::LoadVia> Loads);

private:
  llvm::PHINode *Phi;
  llvm::DominatorTree &DT;
  const llvm::DataLayout &DL;
  const llvm::DenseSet<const llvm::Value *> &ExtraRoots;
  // (value, first incoming index of group) -> value-level PHI carrying the value to the merge block
  llvm::DenseMap<std::pair<llvm::Value *, unsigned>, llvm::Value *> Carried;

  // Incoming pointer I, with leading casts stripped if it is rooted at an alloca/argument.
  llvm::Value *getBase(unsigned I) const;
  bool canRematerialize(llvm::Value *V) const;
  llvm::Value *rematerialize(llvm::IRBuilder<> &B, llvm::Value *V, llvm::ArrayRef<unsigned> Incoming);
};

} // namespace IGC
