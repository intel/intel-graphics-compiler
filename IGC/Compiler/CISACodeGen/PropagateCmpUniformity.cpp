
/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PropagateCmpUniformity.hpp"
#include "Compiler/CISACodeGen/helper.h"

#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "Probe/Assertion.h"
#include "debug/DebugMacros.hpp"
#include "common/debug/Debug.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

using namespace llvm;
using namespace IGC;

// Propagate Cmp Uniformity to dominated BBs
#define PASS_FLAG "igc-propagate-cmp-uniformity"
#define PASS_DESCRIPTION "Propagate Cmp Uniformity"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PropagateCmpUniformity, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(WIAnalysis)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PropagateCmpUniformity, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PropagateCmpUniformity::ID = 0;

FunctionPass *IGC::createPropagateCmpUniformityPass() { return new PropagateCmpUniformity(); }

// This pass detects conditional branches comparing a non-uniform value
// against a uniform value for equality, then replaces uses of the
// non-uniform value with the cheaper uniform one in all dominated blocks
// where equality is guaranteed.
PropagateCmpUniformity::PropagateCmpUniformity() : FunctionPass(ID) {
  initializePropagateCmpUniformityPass(*PassRegistry::getPassRegistry());
}
void PropagateCmpUniformity::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<WIAnalysis>();
  AU.addRequired<CodeGenContextWrapper>();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesCFG();
}

bool PropagateCmpUniformity::runOnFunction(Function &F) {
  DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  m_WI = &getAnalysis<WIAnalysis>();

  bool changed = false;
  for (BasicBlock &BB : F) {
    BranchInst *br = dyn_cast<BranchInst>(BB.getTerminator());
    if (!br || !br->isConditional())
      continue;

    CmpInst *cmp = dyn_cast<CmpInst>(br->getCondition());
    if (!cmp)
      continue;

    // Get the true and false branches for equality
    BasicBlock *trueBranch, *falseBranch;
    Value *uniform, *nonUniform;
    if (!getEqualityBranches(cmp, br, trueBranch, falseBranch, uniform, nonUniform))
      continue;

    changed |= replaceNonUniformWithUniform(cmp, nonUniform, uniform, trueBranch, &BB, DT);
  }
  return changed;
}

bool PropagateCmpUniformity::getEqualityBranches(CmpInst *cmp, BranchInst *br, BasicBlock *&trueBranch,
                                                 BasicBlock *&falseBranch, Value *&uniform, Value *&nonUniform) {
  // First check if we have a uniform/non-uniform pair
  if (!getUniformNonUniformPair(cmp->getOperand(0), cmp->getOperand(1), uniform, nonUniform))
    return false;

  // Determine which branch is taken when equality is true
  if (auto *icmp = dyn_cast<ICmpInst>(cmp)) {
    if (icmp->getPredicate() == ICmpInst::ICMP_EQ) {
      trueBranch = br->getSuccessor(0);
      falseBranch = br->getSuccessor(1);
      return true;
    }
    if (icmp->getPredicate() == ICmpInst::ICMP_NE) {
      trueBranch = br->getSuccessor(1);
      falseBranch = br->getSuccessor(0);
      return true;
    }
  } else if (auto *fcmp = dyn_cast<FCmpInst>(cmp)) {
    auto pred = fcmp->getPredicate();
    if (pred == FCmpInst::FCMP_OEQ || pred == FCmpInst::FCMP_UEQ) {
      trueBranch = br->getSuccessor(0);
      falseBranch = br->getSuccessor(1);
      return true;
    }
    if (pred == FCmpInst::FCMP_ONE || pred == FCmpInst::FCMP_UNE) {
      trueBranch = br->getSuccessor(1);
      falseBranch = br->getSuccessor(0);
      return true;
    }
  }
  return false;
}

bool PropagateCmpUniformity::getUniformNonUniformPair(Value *op0, Value *op1, Value *&uniform, Value *&nonUniform) {
  bool op0Uniform = m_WI->isUniform(op0);
  bool op1Uniform = m_WI->isUniform(op1);

  if (op0Uniform == op1Uniform)
    return false;

  if (op0Uniform) {
    uniform = op0;
    nonUniform = op1;
    return true;
  } else {
    uniform = op1;
    nonUniform = op0;
    return true;
  }
}

bool PropagateCmpUniformity::canReplaceUse(Use &U, BasicBlock *trueBranch, BasicBlock *cmpBB, DominatorTree &DT) {
  User *user = U.getUser();
  BasicBlock *useBB = nullptr;
  BasicBlock *incomingBB = nullptr;

  if (PHINode *phi = dyn_cast<PHINode>(user)) {
    useBB = phi->getParent();
    incomingBB = phi->getIncomingBlock(U);

    // Special case: PHI in trueBranch with incoming edge directly from cmpBB.
    // Equality holds on the cmpBB trueBranch edge, so replacing the incoming
    // nonUniform value with uniform is valid — but only when falseBranch (the
    // other successor of cmpBB) is NOT a direct predecessor of trueBranch.
    // If falseBranch also feeds trueBranch, CFGSimplification may later merge
    // an empty falseBranch into cmpBB, producing two incoming edges from the
    // same block and collapsing the PHI incorrectly (e.g. to a zero constant).
    if (incomingBB == cmpBB && useBB == trueBranch) {
      BranchInst *cmpBr = cast<BranchInst>(cmpBB->getTerminator());
      BasicBlock *falseBranch =
          (cmpBr->getSuccessor(0) == trueBranch) ? cmpBr->getSuccessor(1) : cmpBr->getSuccessor(0);
      for (BasicBlock *pred : predecessors(trueBranch))
        if (pred == falseBranch)
          return false;
      return true;
    }

  } else if (Instruction *inst = dyn_cast<Instruction>(user)) {
    useBB = inst->getParent();
    incomingBB = useBB;
  } else {
    return false;
  }

  // trueBranch must have cmpBB as its only predecessor. If trueBranch is a
  // join point with multiple predecessors, it is reachable without going
  // through the equality-proven edge, so the equality may not hold on all
  // paths to the use even if trueBranch dominates it.
  if (trueBranch->getSinglePredecessor() != cmpBB)
    return false;

  // Only replace if trueBranch dominates the use's incoming BB.
  // Together with the single-predecessor check above, this guarantees every
  // path to the use went through the equality-proven cmpBB→trueBranch edge.
  return DT.dominates(trueBranch, incomingBB);
}

bool PropagateCmpUniformity::replaceNonUniformWithUniform(CmpInst *cmp, Value *nonUniform, Value *uniform,
                                                          BasicBlock *trueBranch, BasicBlock *cmpBB,
                                                          DominatorTree &DT) {
  bool changed = false;
  SmallVector<Use *, 8> usesToReplace;
  for (Use &U : nonUniform->uses()) {
    User *user = U.getUser();

    // Skip the original comparison
    if (user == cmp)
      continue;
    // Skip uses in the definition of the uniform value itself
    if (user == uniform)
      continue;

    // Check if this specific use can be safely replaced
    if (canReplaceUse(U, trueBranch, cmpBB, DT)) {
      usesToReplace.push_back(&U);
    }
  }

  // Perform the replacements
  for (Use *use : usesToReplace) {
    use->set(uniform);
    changed = true;

    if (IGC_GET_FLAG_VALUE(EnablePropagateCmpUniformity) == 2) {
      // Debug output printf("cmp ");
      cmp->printAsOperand(outs(), true);
      outs() << ", uniform ";
      uniform->printAsOperand(outs(), true);
      outs() << ", nonuniform ";
      nonUniform->printAsOperand(outs(), true);
      outs() << ", use: ";
      use->getUser()->printAsOperand(outs(), true);
      if (PHINode *phi = dyn_cast<PHINode>(use->getUser())) {
        outs() << " (PHI incoming from BB ";
        phi->getIncomingBlock(*use)->printAsOperand(outs(), false);
        outs() << ")";
      }
      outs() << "\n";
    }
  }

  return changed;
}

#undef PASS_FLAG
#undef PASS_DESCRIPTION
#undef PASS_CFG_ONLY
#undef PASS_ANALYSIS
