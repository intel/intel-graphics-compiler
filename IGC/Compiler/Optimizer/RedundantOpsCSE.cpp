/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "IGC/Compiler/CodeGenPublic.h"
#include "LLVM3DBuilder/BuiltinsFrontend.hpp"
#include "Probe/Assertion.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/PassInfo.h"
#include "llvm/PassRegistry.h"
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

class RedundantOpsCSEPass : public llvm::FunctionPass {
public:
  static char ID;

  RedundantOpsCSEPass();

  llvm::StringRef getPassName() const override { return "RedundantOpsCSEPass"; }

  ////////////////////////////////////////////////////////////////////////
  bool runOnFunction(llvm::Function &F) override;

  ////////////////////////////////////////////////////////////////////////
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

private:
  bool processFunction(llvm::Function &F);
  bool processRedundantOpsInBB(BasicBlock &BB);
  bool processRedundantOpsAcrossBBs(ReversePostOrderTraversal<Function *> &RPOT, DominatorTree &DT);
};

#define PASS_FLAG "redundant-ops-cse"
#define PASS_DESCRIPTION "RedundantOpsCSEPass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RedundantOpsCSEPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(RedundantOpsCSEPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RedundantOpsCSEPass::ID = 0;

// Returns true when V is alive across the entire function and therefore
// CSE cannot extend its live range: Arguments, Constants, and
// GenISA_RuntimeValue intrinsic calls.
static bool isFunctionWideInvariant(Value *V) {
  if (isa<Argument>(V) || isa<Constant>(V))
    return true;
  if (auto *GII = dyn_cast<GenIntrinsicInst>(V))
    return GII->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue;
  return false;
}

static bool hasAllInvariantOperands(Instruction *I) {
  for (Use &U : I->operands()) {
    if (!isFunctionWideInvariant(U.get()))
      return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////
RedundantOpsCSEPass::RedundantOpsCSEPass() : llvm::FunctionPass(ID) {
  initializeRedundantOpsCSEPassPass(*PassRegistry::getPassRegistry());
}

// Start from BinoaryOperator for now, we can expand to other instructions if needed
bool RedundantOpsCSEPass::processRedundantOpsInBB(BasicBlock &BB) {

  std::vector<Instruction *> InstList;
  SmallSetVector<Instruction *, 8> RemoveSet;

  bool modified = false;

  // Build instruction index map for distance computation within this BB.
  DenseMap<Instruction *, unsigned> InstIndex;
  unsigned Idx = 0;
  for (auto &I : BB) {
    InstIndex[&I] = Idx++;
    if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
      InstList.push_back(BO);
    }
  }

  // If we have less than 2 calls, no CSE opportunity
  if (InstList.size() < 2)
    return false;

  unsigned MaxDist = IGC_GET_FLAG_VALUE(RedundantOpsIntraBBMaxDist);

  // For each call, check if there's an earlier equivalent call
  for (size_t i = 1; i < InstList.size(); ++i) {
    Instruction *CurrentInst = InstList[i];

    // Skip if already marked for removal
    if (RemoveSet.count(CurrentInst))
      continue;

    // Look for an earlier call with the same arguments
    for (size_t j = 0; j < i; ++j) {
      Instruction *EarlierInst = InstList[j];

      // Skip if already marked for removal
      if (RemoveSet.count(EarlierInst))
        continue;

      if (CurrentInst->isIdenticalToWhenDefined(EarlierInst) && CurrentInst->hasSameSubclassOptionalData(EarlierInst)) {
        // Distance guard: skip CSE if the two instructions are too far apart
        // within the BB.  Merging distant identical operations extends the
        // result's live range, which increases register pressure and can cause
        // spilling in high-pressure shaders.  MaxDist=0 means unlimited.
        if (MaxDist > 0) {
          unsigned Dist = InstIndex[CurrentInst] - InstIndex[EarlierInst];
          if (Dist > MaxDist)
            continue;
        }

        // Save the redundant instruction for removal
        RemoveSet.insert(CurrentInst);
        // Replace all uses of the current instruction with the earlier one
        CurrentInst->replaceAllUsesWith(EarlierInst);

        modified = true;
        break; // Found a replacement, move to next instruction
      }
    }
  }

  // Now remove redundant instruction
  for (auto *It : RemoveSet) {
    It->eraseFromParent();
  }

  return modified;
}

bool RedundantOpsCSEPass::processRedundantOpsAcrossBBs(ReversePostOrderTraversal<Function *> &RPOT, DominatorTree &DT) {
  unsigned MaxDist = IGC_GET_FLAG_VALUE(RedundantOpsCrossBBMaxDist);

  // Build instruction index map for distance computation.
  // Distance limits prevent pathological live range extensions in high-pressure
  // SIMD32 compute shaders.
  DenseMap<Instruction *, unsigned> InstIndex;
  unsigned Idx = 0;
  for (auto &BB : RPOT) {
    for (auto &I : *BB)
      InstIndex[&I] = Idx++;
  }

  bool modified = false;

  // Collect all BinaryOperator instructions across the function
  std::vector<Instruction *> AllInsts;
  for (auto &BB : RPOT) {
    for (auto &I : *BB) {
      if (auto *BO = dyn_cast<BinaryOperator>(&I)) {
        AllInsts.push_back(BO);
      }
    }
  }

  if (AllInsts.size() < 2)
    return false;

  SmallSetVector<Instruction *, 16> RemoveSet;

  for (size_t i = 1; i < AllInsts.size(); ++i) {
    Instruction *CurrentInst = AllInsts[i];

    // In invariant-only mode, skip instructions with non-invariant operands.
    if (!hasAllInvariantOperands(CurrentInst))
      continue;

    // Skip if already marked for removal
    if (RemoveSet.count(CurrentInst))
      continue;

    for (size_t j = 0; j < i; ++j) {
      Instruction *EarlierInst = AllInsts[j];

      // Skip if already marked for removal
      if (RemoveSet.count(EarlierInst))
        continue;

      // Same BB case is already handled by ProcessRedundantOpsInBB
      if (EarlierInst->getParent() == CurrentInst->getParent())
        continue;

      // Distance guard: skip CSE if the two instructions are too far apart
      // in program order.  Merging distant identical operations extends the
      // result's live range, which increases register pressure and can cause
      // spilling in high-pressure SIMD32 compute shaders.
      // MaxDist=0 means unlimited (reproduces original behavior for testing).
      if (MaxDist > 0) {
        unsigned Dist = InstIndex[CurrentInst] - InstIndex[EarlierInst];
        if (Dist > MaxDist)
          continue;
      }

      if (CurrentInst->isIdenticalToWhenDefined(EarlierInst) && CurrentInst->hasSameSubclassOptionalData(EarlierInst) &&
          DT.dominates(EarlierInst, CurrentInst)) {
        CurrentInst->replaceAllUsesWith(EarlierInst);
        RemoveSet.insert(CurrentInst);
        modified = true;
        break;
      }
    }
  }

  for (auto *It : RemoveSet) {
    It->eraseFromParent();
  }

  return modified;
}

bool RedundantOpsCSEPass::processFunction(llvm::Function &F) {
  bool modified = false;

  ReversePostOrderTraversal<Function *> RPOT(&F);
  // Process each basic block independently
  for (auto &BB : RPOT) {
    modified |= processRedundantOpsInBB(*BB);
  }

  // Process across basic blocks using dominance -- only when explicitly
  // enabled. The invariant-only mode was extending live ranges across
  // BB boundaries in high-pressure SIMD32 CS, causing spill regressions.
  if (IGC_IS_FLAG_ENABLED(EnableRedundantOpsCrossBBCSE)) {
    DominatorTree &DT = getAnalysis<DominatorTreeWrapperPass>().getDomTree();
    modified |= processRedundantOpsAcrossBBs(RPOT, DT);
  }

  return modified;
}

////////////////////////////////////////////////////////////////////////////
bool RedundantOpsCSEPass::runOnFunction(llvm::Function &F) { return processFunction(F); }

////////////////////////////////////////////////////////////////////////
void RedundantOpsCSEPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.setPreservesCFG();
}

////////////////////////////////////////////////////////////////////////
FunctionPass *createRedundantOpsCSEPass() { return new RedundantOpsCSEPass(); }
