/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "RemoveLoopDependency.hpp"

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <vector>

using namespace llvm;
using namespace IGC;

char RemoveLoopDependency::ID = 0;

#define PASS_FLAG "remove-loop-dependency"
#define PASS_DESCRIPTION "Removing of fantom loop dependency."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RemoveLoopDependency, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(RemoveLoopDependency, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

RemoveLoopDependency::RemoveLoopDependency() : FunctionPass(ID) {
  initializeRemoveLoopDependencyPass(*PassRegistry::getPassRegistry());
}

bool RemoveLoopDependency::runOnFunction(Function &F) {
  bool changed = false;
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  for (auto *L : LI.getLoopsInPreorder())
    changed |= processLoop(L);
  return changed;
}

bool RemoveLoopDependency::processLoop(Loop *L) {
  bool changed = false;
  for (auto &PHI : L->getHeader()->phis()) {
    if (dyn_cast<IGCLLVM::FixedVectorType>(PHI.getType()))
      changed |= RemoveDependency(&PHI, L);
  }

  return changed;
}

bool RemoveLoopDependency::RemoveDependency(PHINode *PHI, Loop *L) {
  unsigned elemCount = cast<IGCLLVM::FixedVectorType>(PHI->getType())->getNumElements();
  std::vector<bool> overwritten(elemCount, false);
  Instruction *currentVector = PHI;

  for (unsigned i = 0; i < elemCount; ++i) {
    if (currentVector->getNumUses() != 1)
      return false;

    if (InsertElementInst *IEI = dyn_cast<InsertElementInst>(currentVector->user_back())) {
      if (!L->contains(IEI))
        return false;

      if (ConstantInt *indexConst = dyn_cast<ConstantInt>(IEI->getOperand(2))) {
        uint64_t index = indexConst->getZExtValue();
        if (index >= elemCount || overwritten[index])
          return false;

        overwritten[index] = true;
        currentVector = IEI;
        continue;
      }
    }

    return false;
  }

  Instruction *firstAssignment = PHI->user_back();
  UndefValue *undefVector = UndefValue::get(firstAssignment->getOperand(0)->getType());
  firstAssignment->setOperand(0, undefVector);

  return true;
}
