/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/DependenceAnalysis.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LoopPassManager.h"
#include "llvm/Transforms/Scalar/LoopSimplifyCFG.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopSimplifyCFG.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopSimplifyCFGLegacyPassWrapper::LoopSimplifyCFGLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopSimplifyCFGLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopSimplifyCFGLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto Adaptor = createFunctionToLoopPassAdaptor(LoopSimplifyCFGPass(), /*UseMemorySSA=*/true);
  PreservedAnalyses PA = Adaptor.run(F, FAM);
  return !PA.areAllPreserved();
}

void LoopSimplifyCFGLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<MemorySSAWrapperPass>();
  AU.addPreserved<DependenceAnalysisWrapperPass>();
  getLoopAnalysisUsage(AU);
}

char LoopSimplifyCFGLegacyPassWrapper::ID = 0;

llvm::Pass *createLoopSimplifyCFGPass() { return new LoopSimplifyCFGLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "loop-simplifycfg-legacy-wrapped"
#define PASS_DESCRIPTION "Simplify loop CFG LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(LoopSimplifyCFGLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(LoopSimplifyCFGLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
