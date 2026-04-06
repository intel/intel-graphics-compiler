
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
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
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

bool PropagateCmpUniformity::canReplaceUse(Use &U, BasicBlock *trueBranchBB, BasicBlock *cmpBB, DominatorTree &DT) {
  User *user = U.getUser();
  BasicBlock *useBB = nullptr;
  BasicBlock *incomingBB = nullptr;

  if (PHINode *phi = dyn_cast<PHINode>(user)) {
    useBB = phi->getParent();
    incomingBB = phi->getIncomingBlock(U);
    // If the replacement won't make the PHI uniform because other incoming
    // values remain non-uniform, skip: we would only introduce a SIMD
    // broadcast on the replaced path without gaining uniformity.
    for (unsigned i = 0, e = phi->getNumIncomingValues(); i < e; ++i) {
      if (&phi->getOperandUse(i) == &U)
        continue;
      if (!m_WI->isUniform(phi->getIncomingValue(i)))
        return false;
    }
  } else if (Instruction *inst = dyn_cast<Instruction>(user)) {
    useBB = inst->getParent();
    incomingBB = useBB;
  } else {
    return false;
  }

  // Compute falseBranchBB once for PHI uses; nullptr for non-PHI.
  BasicBlock *falseBranchBB = nullptr;
  if (isa<PHINode>(user)) {
    BranchInst *cmpBr = cast<BranchInst>(cmpBB->getTerminator());
    falseBranchBB = (cmpBr->getSuccessor(0) == trueBranchBB) ? cmpBr->getSuccessor(1) : cmpBr->getSuccessor(0);
  }

  // Special case: PHI in trueBranchBB with incoming edge directly from cmpBB.
  // Equality holds on cmpBB->trueBranchBB, so the replacement is valid — unless
  // falseBranchBB can also reach trueBranchBB, either directly or via intermediate
  // blocks (e.g. created by JumpThreading). In that case a later CFGSimplification
  // would collapse the intermediate blocks back and corrupt the PHI.
  if (incomingBB == cmpBB && useBB == trueBranchBB) {
    // Sub-case: cmpBB's two successors are the same block (both edges to trueBranchBB).
    if (falseBranchBB == trueBranchBB)
      return false;
    // Sub-case: falseBranchBB is a direct predecessor of trueBranchBB.
    for (BasicBlock *pred : predecessors(trueBranchBB))
      if (pred == falseBranchBB)
        return false;
    // Sub-case: falseBranchBB reaches trueBranchBB via intermediate blocks that are
    // dominated by cmpBB (i.e. they lie on the false-branch path). After a later
    // CFGSimplification those intermediates would be merged away, corrupting the PHI.
    for (BasicBlock *pred : predecessors(trueBranchBB))
      if (pred != cmpBB && DT.dominates(cmpBB, pred))
        return false;
    return true;
  }

  // trueBranchBB must have cmpBB as its only predecessor; otherwise equality may
  // not hold on all paths to the use.
  if (trueBranchBB->getSinglePredecessor() != cmpBB)
    return false;

  // For PHI uses: reject if falseBranchBB or cmpBB is also a direct predecessor
  // of useBB. CFGSimplification would later collapse those edges back to cmpBB,
  // overwriting the non-equality incoming value with the uniform constant.
  // (The cmpBB check covers the case where an earlier CFGSimplification pass
  // already merged trueBranchBB away, leaving cmpBB with a direct false edge to
  // useBB where the equality guarantee does not hold.)
  if (falseBranchBB) {
    for (BasicBlock *pred : predecessors(useBB))
      if (pred == falseBranchBB || pred == cmpBB)
        return false;
  }

  // Only replace if trueBranchBB dominates the use's incomingBB.
  // Together with the single-predecessor check above, this guarantees every
  // path to the use went through the equality-proven cmpBB->trueBranchBB edge.
  return DT.dominates(trueBranchBB, incomingBB);
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
