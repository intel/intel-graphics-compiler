/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "MoveStaticAllocas.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstIterator.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-move-static-allocas"
#define PASS_DESCRIPTION "Move static allocas to entry basic block of the function"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MoveStaticAllocasLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(MoveStaticAllocasLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MoveStaticAllocasLPM::ID = 0;

// This function is copied from LLVM's InlineFunction.
static bool allocaWouldBeStaticInEntry(const AllocaInst *AI) {
  return isa<Constant>(AI->getArraySize()) && !AI->isUsedWithInAlloca();
}

MoveStaticAllocasLPM::MoveStaticAllocasLPM() : FunctionPass(ID) {
  initializeMoveStaticAllocasLPMPass(*PassRegistry::getPassRegistry());
}

bool MoveStaticAllocas::run(llvm::Function &F) {
  std::vector<AllocaInst *> staticAllocas;

  // Note: we are also moving static allocas that are already in the entry BB.
  for (auto inst = inst_begin(F), endInst = inst_end(F); inst != endInst; ++inst) {
    if (AllocaInst *allocaInst = dyn_cast<AllocaInst>(&*inst)) {
      if (allocaInst->isStaticAlloca() || allocaWouldBeStaticInEntry(allocaInst)) {
        staticAllocas.push_back(allocaInst);
      }
    }
  }

  for (auto I = staticAllocas.rbegin(), E = staticAllocas.rend(); I != E; ++I) {
    IGCLLVM::moveBefore((*I), &*F.getEntryBlock().getFirstInsertionPt());
  }

  return staticAllocas.size() > 0;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses MoveStaticAllocasNPM::run(Function &F, FunctionAnalysisManager &) {
  bool changed = MoveStaticAllocas().run(F);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
