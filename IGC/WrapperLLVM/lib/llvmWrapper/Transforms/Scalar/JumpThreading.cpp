/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Scalar/JumpThreading.h"
#include "llvm/Analysis/DomTreeUpdater.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LazyValueInfo.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/Transforms/Scalar.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/JumpThreading.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

JumpThreadingPassWrapper::JumpThreadingPassWrapper() : FunctionPass(ID), Threshold(-1) {
  initializeJumpThreadingPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}
JumpThreadingPassWrapper::JumpThreadingPassWrapper(int Threshold) : FunctionPass(ID), Threshold(Threshold) {
  initializeJumpThreadingPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}
#if LLVM_VERSION_MAJOR <= 14
JumpThreadingPassWrapper::JumpThreadingPassWrapper(bool InsertFreezeWhenUnfoldingSelect, int Threshold)
    : FunctionPass(ID) {
  initializeJumpThreadingPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerLoopAnalyses(LAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerModuleAnalyses(MAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}
#endif
bool JumpThreadingPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  // Run the New Pass Manager implementation of the pass.
  JumpThreadingPass Implementation(Threshold);
  auto PAs = Implementation.run(F, FAM);
  return PAs.areAllPreserved() ? false : true;
}

void JumpThreadingPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<DominatorTreeWrapperPass>();
#if LLVM_VERSION_MAJOR < 16
  // Workaround. Avoid Dom Tree preservation for LLVM 16+ when using wrapper.

  // For some reason after running JumpThreadingPass via wrapper
  // the Dom Tree is not always valid, at least when accessed e.g by Natural Loop Information pass, which results in
  // segfault.

  // I didn't manage to find any difference when it comes to execution of Jump Threading Pass implementation
  // and additionally the dumps of Dom Tree seem to be same before and after running the pass on both: wrapper and
  // non-wrapper versions.

  // This workaround disabled Dom Tree preservation when using wrapper on LLVM 16 where issue exists.
  AU.addPreserved<DominatorTreeWrapperPass>();
#endif
  AU.addRequired<AAResultsWrapperPass>();
  AU.addRequired<LazyValueInfoWrapperPass>();
  AU.addPreserved<LazyValueInfoWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<TargetTransformInfoWrapperPass>();
}

char JumpThreadingPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedJumpThreadingPass(int Threshold) {
#if LLVM_VERSION_MAJOR >= 16
  return new JumpThreadingPassWrapper(Threshold);
#else
  return llvm::createJumpThreadingPass(Threshold);
#endif
}

#if LLVM_VERSION_MAJOR <= 14
FunctionPass *createLegacyWrappedJumpThreadingPass(bool InsertFreezeWhenUnfoldingSelect, int Threshold) {
  return llvm::createJumpThreadingPass(InsertFreezeWhenUnfoldingSelect, Threshold);
}
#endif
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "jump-threading-legacy-wrapped"
#define PASS_DESCRIPTION "Jump Threading LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(JumpThreadingPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LazyValueInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_END(JumpThreadingPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
