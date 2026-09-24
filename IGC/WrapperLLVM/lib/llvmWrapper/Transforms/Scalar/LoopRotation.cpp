/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Analysis/MemorySSA.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"

#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/LCSSA.h"
#include "llvm/Transforms/Utils/LoopRotationUtils.h"
#include "llvm/Transforms/Utils/LoopSimplify.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LoopRotation.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LoopRotateLegacyPassWrapper::LoopRotateLegacyPassWrapper(bool EnableHeaderDuplication, bool PrepareForLTO,
                                                         int MaxHeaderSize)
    : FunctionPass(ID), EnableHeaderDuplication(EnableHeaderDuplication), PrepareForLTO(PrepareForLTO),
      MaxHeaderSize(MaxHeaderSize) {
  initializeLoopRotateLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool LoopRotateLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  auto *DefaultOpt = static_cast<cl::opt<unsigned> *>(cl::getRegisteredOptions().lookup("rotation-max-header-size"));
  unsigned DefaultThreshold = DefaultOpt ? DefaultOpt->getValue() : 16;
  unsigned MaxThreshold = !EnableHeaderDuplication ? 0 : MaxHeaderSize < 0 ? DefaultThreshold : unsigned(MaxHeaderSize);

  FunctionPassManager Canonicalize;
  Canonicalize.addPass(LoopSimplifyPass());
  Canonicalize.addPass(LCSSAPass());
  bool Changed = !Canonicalize.run(F, FAM).areAllPreserved();

  auto &LI = FAM.getResult<LoopAnalysis>(F);
  auto &DT = FAM.getResult<DominatorTreeAnalysis>(F);
  auto &SE = FAM.getResult<ScalarEvolutionAnalysis>(F);
  auto &AC = FAM.getResult<AssumptionAnalysis>(F);
  auto &TTI = FAM.getResult<TargetIRAnalysis>(F);
  auto &TLI = FAM.getResult<TargetLibraryAnalysis>(F);
  const SimplifyQuery SQ(F.getParent()->getDataLayout(), &TLI, &DT, &AC);

  auto Loops = LI.getLoopsInPreorder();
  for (Loop *L : llvm::reverse(Loops)) {
    unsigned Threshold = hasVectorizeTransformation(L) == TM_ForcedByUser ? DefaultThreshold : MaxThreshold;
    Changed |= LoopRotation(L, &LI, &TTI, &AC, &DT, &SE, nullptr, SQ, false, Threshold, false, PrepareForLTO);
  }

  if (Changed)
    FAM.invalidate(F, PreservedAnalyses::none());
  return Changed;
}

void LoopRotateLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<AssumptionCacheTracker>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
  AU.addPreserved<MemorySSAWrapperPass>();

  AU.addPreserved<LazyBlockFrequencyInfoPass>();
  AU.addPreserved<LazyBranchProbabilityInfoPass>();
}

char LoopRotateLegacyPassWrapper::ID = 0;

llvm::Pass *createLegacyWrappedLoopRotatePass(int MaxHeaderSize, bool PrepareForLTO) {
#if LLVM_VERSION_MAJOR < 22
  return llvm::createLoopRotatePass(MaxHeaderSize, PrepareForLTO);
#else
  bool EnableHeaderDuplication = (MaxHeaderSize != 0);
  return new LoopRotateLegacyPassWrapper(EnableHeaderDuplication, PrepareForLTO, MaxHeaderSize);
#endif
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "loop-rotate-legacy-wrapped"
#define PASS_DESCRIPTION "Rotate Loops LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LoopRotateLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AssumptionCacheTracker)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetTransformInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(MemorySSAWrapperPass)
IGC_INITIALIZE_PASS_END(LoopRotateLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
