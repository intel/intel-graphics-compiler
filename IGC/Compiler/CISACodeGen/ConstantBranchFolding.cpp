/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// ConstantBranchFolding.cpp - Fold branches on constant conditions, and deletes the blocks that become unreachable
// Optionally merges blocks into their single predecessors if the predecessor only branches to them.
//
// InstCombine, EarlyCSE, InstSimplify and LowerSIMDSize can turn a branch condition into a constant after the last
// SimplifyCFG that would clean it up. EmitVISA then emits the dead edge as a predicated goto on an immediate flag, with
// the dead block, its phi copies and a join, and the finalizer keeps them: the block is still reachable in its CFG.
//
// SimplifyCFG would remove them, but the legacy pass ignores SimplifyCondBranch / FoldTwoEntryPHINode = false (its
// runOnFunction sets both to true), so running it this late re-forms the switches LowerSwitch just lowered and
// speculates blocks into selects. This pass only does the constant fold and deletes what it made dead.
//
// It deliberately does not use llvm::removeUnreachableBlocks (or UnreachableHandling, which calls it). That folds
// terminators too, but its markAliveBlocks also turns stores to null/undef, false assumes and the code after noreturn
// calls into `unreachable`, on reachable paths, which then end without an EOT. UnreachableHandling repairs that with a
// return, but only before the shader-stage lowering, which puts outputs and the EOT on the existing return; this pass
// also runs after it. So the fold is ConstantFoldTerminator and the deletion EliminateUnreachableBlocks, which only
// removes blocks: it creates no `unreachable`, and UnreachableHandling does not need to run again after it.
//
// Optionally (ConstantBranchFoldingMask bit 2, a behaviour bit for every location the pass runs at) it then merges
// each block into its single predecessor when that predecessor branches only to it. The finalizer merges such chains
// anyway before scheduling, so this only changes what the IGC passes between here and EmitVISA see. It cannot create or
// remove a critical edge (the merged edge is never critical) nor new constant branches, so it needs no iteration.
//
//===----------------------------------------------------------------------===//

#include "Compiler/CISACodeGen/ConstantBranchFolding.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"
#include "common/igc_regkeys.hpp"

using namespace llvm;
using namespace IGC;

namespace IGC {

class ConstantBranchFolding : public FunctionPass {
public:
  static char ID;
  ConstantBranchFolding() : FunctionPass(ID) { initializeConstantBranchFoldingPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &F) override;

  StringRef getPassName() const override { return "Constant Branch Folding"; }
};

bool ConstantBranchFolding::runOnFunction(Function &F) {
  bool Changed = false;
  bool Folded;
  // Removing a dead edge can collapse a phi to a constant that another branch tests, so repeat until nothing folds.
  do {
    Folded = false;
    for (BasicBlock &BB : F)
      Folded |= ConstantFoldTerminator(&BB, /*DeleteDeadConditions=*/true);
    // Only delete unreachable blocks after a fold, so that a function with nothing to fold is left untouched.
    if (Folded)
      EliminateUnreachableBlocks(F);
    Changed |= Folded;
  } while (Folded);

  // Behaviour bit: wherever the pass runs, also merge single-predecessor chains.
  if (IGC_GET_FLAG_VALUE(ConstantBranchFoldingMask) & static_cast<unsigned>(ConstantBranchFoldingOpt::MergeBlocks)) {
    for (auto It = F.begin(); It != F.end();) {
      BasicBlock *BB = &*It++; // BB is erased when it is merged into its predecessor
      Changed |= MergeBlockIntoPredecessor(BB);
    }
  }
  return Changed;
}

FunctionPass *createConstantBranchFoldingPass() { return new ConstantBranchFolding(); }

} // namespace IGC

char ConstantBranchFolding::ID = 0;

#define PASS_FLAG "igc-constant-branch-folding"
#define PASS_LABEL "Constant Fold cond branches with constant conditions, and delete unreachable blocks"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ConstantBranchFolding, PASS_FLAG, PASS_LABEL, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(ConstantBranchFolding, PASS_FLAG, PASS_LABEL, PASS_CFG_ONLY, PASS_ANALYSIS)
