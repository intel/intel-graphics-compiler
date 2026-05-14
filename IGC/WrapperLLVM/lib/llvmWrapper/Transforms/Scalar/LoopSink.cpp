/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LoopSink.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/BlockFrequencyInfo.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopSink.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopSinkLegacyPassWrapper::LoopSinkLegacyPassWrapper() : FunctionPass(ID) {
  initializeLoopSinkLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool LoopSinkLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  LoopSinkPass Pass;
  PreservedAnalyses PA = Pass.run(F, FAM);
  return !PA.areAllPreserved();
}

void LoopSinkLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<BlockFrequencyInfoWrapperPass>();
  getLoopAnalysisUsage(AU);
  AU.addRequired<MemorySSAWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();
}

char LoopSinkLegacyPassWrapper::ID = 0;

FunctionPass *createLegacyWrappedLoopSinkPass() { return new LoopSinkLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loop-sink-legacy-wrapped"
#define PASS_DESCRIPTION "Loop Sink LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopSinkLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(BlockFrequencyInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(LoopSinkLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
