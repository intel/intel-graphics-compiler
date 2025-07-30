/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include <llvm/Support/Debug.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/Optimizer/HoistConvOpToDom.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Probe/Assertion.h"

#define DEBUG_TYPE "HoistConvOpToDom"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-hoist-conv-op-to-dom"
#define PASS_DESCRIPTION "Hoist conversion operations to dominator"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HoistConvOpToDom, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(HoistConvOpToDom, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {
char HoistConvOpToDom::ID = 0;

HoistConvOpToDom::HoistConvOpToDom() : FunctionPass(ID) {
  initializeHoistConvOpToDomPass(*PassRegistry::getPassRegistry());
}

BasicBlock *HoistConvOpToDom::findNearestCommonDominator(const PHINode &PHI) const {
  BasicBlock *CommonDominator = nullptr;
  for (unsigned i = 0; i < PHI.getNumIncomingValues(); ++i) {
    BasicBlock *IncomingBB = PHI.getIncomingBlock(i);
    if (CommonDominator == nullptr) {
      CommonDominator = IncomingBB;
      continue;
    }
    CommonDominator = m_DT->findNearestCommonDominator(CommonDominator, IncomingBB);
  }
  return CommonDominator;
}

bool HoistConvOpToDom::runOnFunction(Function &F) {
  m_CGCtx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  bool Changed = false;

  for (auto &BB : F) {
    for (auto &PHI : make_early_inc_range(BB.phis())) {
      Value *FirstIncoming = PHI.getIncomingValue(0);
      CastInst *FirstCast = dyn_cast<CastInst>(FirstIncoming);
      if (!FirstCast)
        continue;
      LLVM_DEBUG(dbgs() << "HoistConvOpToDom: Found PHI with cast: " << *FirstCast << "\n");

      SmallPtrSet<Instruction *, 4> ToUpdate;
      bool AllSame = true;
      for (unsigned i = 1; i < PHI.getNumIncomingValues(); ++i) {
        Value *Incoming = PHI.getIncomingValue(i);
        if (Incoming == FirstCast)
          continue;

        Instruction *IncomingInst = dyn_cast<Instruction>(Incoming);
        if (!IncomingInst || !IncomingInst->isIdenticalTo(FirstCast)) {
          AllSame = false;
          break;
        }

        ToUpdate.insert(cast<Instruction>(Incoming));
      }
      if (!AllSame)
        continue;

      // Corner case: PHI with incoming values equal to the same first cast.
      // examples:
      // %phi = phi i32 [%conv1, %then]
      // %phi = phi i32 [%conv1, %then], [%conv1, %else]
      // etc...
      // just replace %phi with %conv1 and continue
      if (ToUpdate.empty()) {
        PHI.replaceAllUsesWith(FirstCast);
        PHI.eraseFromParent();
        Changed = true;
        continue;
      }

      auto *CommonDominator = findNearestCommonDominator(PHI);
      if(FirstCast->getParent() != CommonDominator)
        FirstCast->moveBefore(CommonDominator->getTerminator());

      for (auto *I : ToUpdate) {
        LLVM_DEBUG(dbgs() << "HoistConvOpToDom: Replacing " << *I << " with " << *FirstCast << "\n");
        I->replaceAllUsesWith(FirstCast);
        I->eraseFromParent();
      }

      PHI.replaceAllUsesWith(FirstCast);
      PHI.eraseFromParent();
      Changed = true;
    }
  }
  return Changed;
}
} // namespace IGC
