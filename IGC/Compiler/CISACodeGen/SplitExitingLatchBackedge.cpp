/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// LoopRotate does not rotate a loop whose latch already exits: it treats the loop as rotated, unless the latch was
// just simplified or a header phi is only used through the header's exit. A loop tested at both the top and the
// bottom (header and latch both exiting) is therefore left with its exit test in the header. Splitting the latch's
// back-edge gives such a loop an unconditional latch, so LoopRotate moves the header's test to the bottom.
// This pass splits only those back-edges, instead of every critical edge in the function as BreakCriticalEdges does.

#include "Compiler/CISACodeGen/SplitExitingLatchBackedge.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/InitializePasses.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/Pass.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvmWrapper/IR/Instructions.h"

#include <utility>

using namespace llvm;
using namespace IGC;

namespace IGC {

class SplitExitingLatchBackedge : public FunctionPass {
public:
  static char ID;

  SplitExitingLatchBackedge() : FunctionPass(ID) {
    initializeSplitExitingLatchBackedgePass(*PassRegistry::getPassRegistry());
  }

  bool runOnFunction(Function &F) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addPreserved<DominatorTreeWrapperPass>();
    AU.addPreserved<LoopInfoWrapperPass>();
  }

  StringRef getPassName() const override { return "SplitExitingLatchBackedge"; }
};

bool SplitExitingLatchBackedge::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  auto &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  // Collect first: splitting changes the latch, and the loop list must not change while it is walked.
  SmallVector<std::pair<BasicBlock *, BasicBlock *>, 8> Edges;
  for (Loop *L : LI.getLoopsInPreorder()) {
    BasicBlock *Header = L->getHeader();
    BasicBlock *Latch = L->getLoopLatch();
    // Several back-edges: LoopSimplify, which LoopRotate requires, gives such a loop an unconditional latch, which
    // does not block rotation (LoopCanonicalization above already does the same). A single-block loop is already
    // tested at the bottom.
    if (!Latch || Latch == Header)
      continue;
    IGCLLVM::TerminatorInst *LatchTerm = Latch->getTerminator();
    if (!isa<IGCLLVM::CondBrInst>(LatchTerm))
      continue;
    // A loop whose header does not exit is never rotated, and one whose latch does not exit already gets rotated.
    if (!L->isLoopExiting(Header) || !L->isLoopExiting(Latch))
      continue;
    // Only a critical back-edge needs a new block.
    if (!isCriticalEdge(LatchTerm, Header))
      continue;
    Edges.push_back({Latch, Header});
  }

  for (auto &[Latch, Header] : Edges)
    SplitCriticalEdge(Latch, Header, CriticalEdgeSplittingOptions(&DT, &LI));
  return !Edges.empty();
}

FunctionPass *createSplitExitingLatchBackedgePass() { return new SplitExitingLatchBackedge(); }

} // namespace IGC

char SplitExitingLatchBackedge::ID = 0;

#define PASS_FLAG "igc-split-exiting-latch-backedge"
#define PASS_LABEL "Split the backedge of loops whose loop header and loop latch both exit"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitExitingLatchBackedge, PASS_FLAG, PASS_LABEL, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(SplitExitingLatchBackedge, PASS_FLAG, PASS_LABEL, PASS_CFG_ONLY, PASS_ANALYSIS)
