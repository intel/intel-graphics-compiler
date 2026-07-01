/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include <optional>

namespace IGC {
// BranchToSelect linearizes small, memory-free, acyclic branch regions into
// straight-line code: cheap and known speculatable instructions are hoisted into the
// predecessor, and the merge-block PHIs are turned into SELECTs.
//
// It is built from a single minimal triangle/diamond primitive that is applied
// bottom-up to a fixpoint, so nested patterns (e.g. a short-circuit `||` chain)
// collapse one inner unit at a time. Interior PHIs need no special handling:
// they belong to inner diamonds that are resolved into selects first.
//
// Triangle:
//   P: vP = ...; br cond, T, MB
//   T: vT = ...; br MB
//   MB: phi [vT,T],[vP,P]
//
// becomes:
//   P: vP = ...; vT = ...; vC = select(cond, vT, vP); br MB
//   MB: phi [vC,P]
//
// Diamond:
//   P: vP = ...; br cond, T, F
//   T: vT = ...; br MB
//   F: vF = ...; br MB
//   MB: phi [vT,T],[vF,F]
//
// becomes:
//   P: vP = ...; vT = ...; vF = ...; vC = select(cond, vT, vF); br MB
//   MB: phi [vC,P]
//
// SimplifyCFG will clean up the single incoming value phis
//
// A foldable successor may also be a shared, empty landing pad LP (a
// critical-edge-split block reached from P and others), in which case P is
// peeled off it:
//   P:  vP = ...; br cond, LP, MB
//   LP: br MB // (LP has no insts other than branch, and has predecessors other than P)
//   MB: phi [0,LP],[vP,P]
//
// becomes:
//   P: vP = ...; vC = select(cond, 0, vP); br MB
//   LP: br MB (one less predecessor)
//   MB: phi [vC,P], [0,LP]
//
// Iterative approach used to potentially peel all of LP's predecessors.
class BranchToSelect : public llvm::FunctionPass {
public:
  static char ID;
  BranchToSelect();
  ~BranchToSelect() {}

  llvm::StringRef getPassName() const override { return "BranchToSelect"; }

  bool runOnFunction(llvm::Function &F) override;

private:
  enum class SuccessorKind {
    None,      // not a foldable successor
    Private,   // sole successor of P; hoist its body and delete it
    SharedPad, // empty pass-through with other predecessors; peel P off it
  };

  // Attempt to convert the triangle/diamond rooted at conditional block P.
  bool tryConvert(llvm::BasicBlock *P);

  // Classify a branch successor Succ of P as a private foldable block, a shared
  // landing pad, or neither. Both foldable kinds end in an unconditional branch.
  // For a Private successor, BodyCost is set to the summed latency cost of the
  // body that would be hoisted; it is 0 for the other kinds.
  SuccessorKind classifySuccessor(llvm::BasicBlock *Succ, llvm::BasicBlock *P, unsigned &BodyCost) const;

  // Move all non-terminator instructions of Blk to just before P's terminator.
  void hoistInto(llvm::BasicBlock *Blk, llvm::BasicBlock *P);

  // Approximate assigned latency cost of speculatively executing I, or nullopt if I must not be speculated.
  std::optional<unsigned> speculationLatencyCost(const llvm::Instruction *I) const;

  // Per-fold budget: bounds the cost of a single hoisted successor. Set with regkey.
  unsigned m_maxSpeculatedCost = 0;

  // Cumulative budget: bounds the cost accreted into one linearized region across
  // successive folds. Set with regkey.
  unsigned m_maxRegionCost = 0;

  // Speculation cost currently residing in each block's linearized region,
  // accumulated across folds and bounded by m_maxRegionCost. Reset per function.
  llvm::DenseMap<const llvm::BasicBlock *, unsigned> m_regionCost;
};

llvm::FunctionPass *createBranchToSelectPass();
} // namespace IGC
