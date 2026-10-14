/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
///
/// Sinks a RayQuery allocation and its query operations in the same block
/// into a conditional successor that dominates all remaining uses of the
/// handle and operation results. The successor must have no other predecessor.
/// This skips query work on paths that do not use it.
///
/// Supported operations only modify their own query state, though they may
/// read the acceleration structure. The pass requires direct handle uses by
/// these operations and rejects other memory writes after the allocation in
/// the source block. For example:
///
///   A:
///     %q    = AllocateRayQuery(...)
///     %mask = select i1 %hit, i32 32, i32 0
///     TraceRayInlineHL(%q, ..., %mask, ...)
///     %p    = TraceRaySyncProceedHL(%q)
///     br i1 %hit, label %S, label %M
///   S:
///     %t    = TraceRayInlineRayInfo(%q, ...)
///
/// becomes
///
///   A:
///     br i1 %hit, label %S, label %M
///   S:
///     %q    = AllocateRayQuery(...)
///     TraceRayInlineHL(%q, ..., i32 32, ...)
///     %p    = TraceRaySyncProceedHL(%q)
///     %t    = TraceRayInlineRayInfo(%q, ...)
///
/// Operand computations can move too if they have no side effects and all
/// their uses are dominated by the successor. Moved selects on the branch
/// condition fold to the value selected by that edge.
///
/// Run before RayQueryCheck/Release placement and TraceRayInline lowering.
/// Lowering moves the query state into RTStack memory accessed by plain
/// loads, stores and RTA messages. The query operations no longer share the
/// handle, so they can no longer be identified and sunk together.
///
//===----------------------------------------------------------------------===//

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"

using namespace IGC;
using namespace llvm;

class RayQuerySinkingPass : public FunctionPass {
public:
  RayQuerySinkingPass() : FunctionPass(ID) { initializeRayQuerySinkingPassPass(*PassRegistry::getPassRegistry()); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
  }

  bool runOnFunction(Function &F) override;
  StringRef getPassName() const override { return "RayQuerySinkingPass"; }

  static char ID;

private:
  bool sinkRayQuery(AllocateRayQueryIntrinsic *RayQuery);
  void sinkOperands(BasicBlock *BB, BasicBlock *Target, SmallPtrSetImpl<Instruction *> &Moved);
  void foldBranchCondition(IGCLLVM::CondBrInst *Br, BasicBlock *Target, const SmallPtrSetImpl<Instruction *> &Moved);

  DominatorTree *DT = nullptr;
};

char RayQuerySinkingPass::ID = 0;

// Register the pass with igc-opt.
#define PASS_FLAG "igc-rayquery-sinking"
#define PASS_DESCRIPTION "Sink RayQuery work into the branch that uses it"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RayQuerySinkingPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RayQuerySinkingPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

// Supported non-convergent intrinsics access the query in operand 0. They may
// read the acceleration structure, but only modify that query's state.
static bool hasQueryLocalEffects(const Instruction *I) {
  auto *RQI = dyn_cast<RayQueryIntrinsicBase>(I);
  if (!RQI || RQI->isConvergent())
    return false;

  switch (RQI->getIntrinsicID()) {
  case GenISAIntrinsic::GenISA_TraceRayInlineHL:
  case GenISAIntrinsic::GenISA_TraceRaySyncProceedHL:
  case GenISAIntrinsic::GenISA_TraceRayInlineAbort:
  case GenISAIntrinsic::GenISA_TraceRayInlineCommittedStatus:
  case GenISAIntrinsic::GenISA_TraceRayInlineCandidateType:
  case GenISAIntrinsic::GenISA_TraceRayInlineRayInfo:
  case GenISAIntrinsic::GenISA_TraceRayInlineCommitNonOpaqueTriangleHit:
  case GenISAIntrinsic::GenISA_TraceRayInlineCommitProceduralPrimitiveHit:
    return true;
  default:
    return false;
  }
}

// The block in which a use reads its value. PHI operands are read at the end
// of the incoming block.
static BasicBlock *getUseBlock(const Use &U) {
  auto *UserI = cast<Instruction>(U.getUser());
  if (auto *PN = dyn_cast<PHINode>(UserI))
    return PN->getIncomingBlock(U);
  return UserI->getParent();
}

static bool allUseBBsDominatedBy(const Instruction *I, const BasicBlock *Target, const DominatorTree &DT) {
  return llvm::all_of(I->uses(), [&](const Use &U) { return DT.dominates(Target, getUseBlock(U)); });
}

// Collect the allocation and query operations in its block into Group.
// ObserverBlocks records uses of the handle or Group's results outside Group.
// Reject unsupported handle uses and queries with no uses outside Group.
static bool collectQueryGroupAndObserverBlks(AllocateRayQueryIntrinsic *RayQuery, SmallPtrSetImpl<Instruction *> &Group,
                                             SmallVectorImpl<BasicBlock *> &ObserverBlocks) {
  BasicBlock *BB = RayQuery->getParent();

  // Require direct uses as operand 0 of supported intrinsics. Handle aliases
  // through PHIs, selects or memory, and other uses, are not supported.
  Group.insert(RayQuery);
  for (const Use &U : RayQuery->uses()) {
    auto *UserI = cast<Instruction>(U.getUser());
    if (U.getOperandNo() != 0 || !hasQueryLocalEffects(UserI))
      return false;
    if (UserI->getParent() == BB)
      Group.insert(UserI);
    else
      ObserverBlocks.push_back(UserI->getParent());
  }

  // Include uses outside Group of results such as Proceed's return value.
  for (auto *I : Group) {
    for (const Use &U : I->uses()) {
      BasicBlock *UseBB = getUseBlock(U);
      if (UseBB == BB && Group.count(cast<Instruction>(U.getUser())))
        continue;
      ObserverBlocks.push_back(UseBB);
    }
  }

  // Without external uses, there is no target to choose.
  return !ObserverBlocks.empty();
}

bool RayQuerySinkingPass::runOnFunction(Function &F) {
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  SmallVector<AllocateRayQueryIntrinsic *, 4> RayQueries;
  for (auto &I : instructions(F)) {
    if (auto *RayQuery = dyn_cast<AllocateRayQueryIntrinsic>(&I))
      RayQueries.push_back(RayQuery);
  }

  bool Changed = false;
  for (auto *RayQuery : RayQueries) {
    // Keep sinking through nested branches while the same checks hold.
    while (sinkRayQuery(RayQuery))
      Changed = true;
  }

  return Changed;
}

bool RayQuerySinkingPass::sinkRayQuery(AllocateRayQueryIntrinsic *RayQuery) {
  BasicBlock *BB = RayQuery->getParent();
  // Skip unreachable blocks.
  if (!DT->isReachableFromEntry(BB))
    return false;

  auto *Br = dyn_cast<IGCLLVM::CondBrInst>(BB->getTerminator());
  if (!Br || Br->getSuccessor(0) == Br->getSuccessor(1))
    return false;

  SmallPtrSet<Instruction *, 8> Group;
  SmallVector<BasicBlock *, 8> ObserverBlocks;
  if (!collectQueryGroupAndObserverBlks(RayQuery, Group, ObserverBlocks))
    return false;

  // Choose a successor with no other predecessor that dominates every use
  // outside Group. Uses may be in the successor or in blocks it dominates.
  BasicBlock *Target = nullptr;
  for (BasicBlock *Succ : successors(BB)) {
    if (Succ == BB || Succ->getSinglePredecessor() != BB)
      continue;
    if (llvm::all_of(ObserverBlocks, [&](BasicBlock *ObsBB) { return DT->dominates(Succ, ObsBB); })) {
      Target = Succ;
      break;
    }
  }
  if (!Target)
    return false;

  // Group does not iterate in program order, so walk BB to collect it in
  // program order: the allocation must precede its users, and the query
  // operations must keep their relative order. The same walk rejects other
  // memory writes after the allocation, since sinking would move acceleration
  // structure reads past them.
  SmallVector<Instruction *, 8> OrderedGroup;
  for (Instruction &I : make_range(RayQuery->getIterator(), BB->end())) {
    if (Group.count(&I))
      OrderedGroup.push_back(&I);
    else if (!I.isTerminator() && I.mayWriteToMemory())
      return false;
  }

  Instruction *InsertPt = &*Target->getFirstInsertionPt();
  for (auto *I : OrderedGroup)
    IGCLLVM::moveBefore(I, InsertPt);

  SmallPtrSet<Instruction *, 16> Moved(OrderedGroup.begin(), OrderedGroup.end());
  sinkOperands(BB, Target, Moved);
  foldBranchCondition(Br, Target, Moved);

  return true;
}

// Sink eligible operands from BB when all uses are dominated by Target and
// at least one user has moved. Walk backward to place definitions before users.
void RayQuerySinkingPass::sinkOperands(BasicBlock *BB, BasicBlock *Target, SmallPtrSetImpl<Instruction *> &Moved) {
  bool SeenStore = false;
  for (Instruction &I : llvm::make_early_inc_range(llvm::reverse(*BB))) {
    if (I.mayWriteToMemory())
      SeenStore = true;

    if (I.isTerminator() || isa<PHINode>(I) || isa<AllocaInst>(I) || I.use_empty() || I.mayHaveSideEffects())
      continue;
    if (auto *CI = dyn_cast<CallInst>(&I); CI && CI->isConvergent())
      continue;
    // Memory reads may move only if no memory write follows them in BB.
    if (I.mayReadFromMemory() && SeenStore)
      continue;

    bool FeedsMoved = llvm::any_of(I.users(), [&](User *U) { return Moved.count(cast<Instruction>(U)); });
    if (!FeedsMoved || !allUseBBsDominatedBy(&I, Target, *DT))
      continue;

    IGCLLVM::moveBefore(&I, &*Target->getFirstInsertionPt());
    Moved.insert(&I);
  }
}

// Fold moved selects whose condition is exactly Br's condition. Target's sole
// incoming edge determines the selected value, e.g. a constant ray mask.
void RayQuerySinkingPass::foldBranchCondition(IGCLLVM::CondBrInst *Br, BasicBlock *Target,
                                              const SmallPtrSetImpl<Instruction *> &Moved) {
  Value *Cond = Br->getCondition();
  if (isa<Constant>(Cond))
    return;

  bool CondIsTrue = Target == Br->getSuccessor(0);

  // Leave set entries intact while iterating; erased pointers are not reused.
  for (auto *I : Moved) {
    if (auto *Sel = dyn_cast<SelectInst>(I); Sel && Sel->getCondition() == Cond) {
      Sel->replaceAllUsesWith(CondIsTrue ? Sel->getTrueValue() : Sel->getFalseValue());
      Sel->eraseFromParent();
    }
  }
}

namespace IGC {
Pass *createRayQuerySinkingPass() { return new RayQuerySinkingPass(); }
} // namespace IGC
