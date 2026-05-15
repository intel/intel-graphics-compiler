/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/LowerConstantIntrinsics.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LowerConstantIntrinsics.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LowerConstantIntrinsicsLegacyPassWrapper::LowerConstantIntrinsicsLegacyPassWrapper() : FunctionPass(ID) {
  initializeLowerConstantIntrinsicsLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool LowerConstantIntrinsicsLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  LowerConstantIntrinsicsPass Pass;
  PreservedAnalyses PA = Pass.run(F, FAM);
  return !PA.areAllPreserved();
}

void LowerConstantIntrinsicsLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addPreserved<DominatorTreeWrapperPass>();
}

char LowerConstantIntrinsicsLegacyPassWrapper::ID = 0;

FunctionPass *createLegacyWrappedLowerConstantIntrinsicsPass() {
  return new LowerConstantIntrinsicsLegacyPassWrapper();
}

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "lower-constant-intrinsics-legacy-wrapped"
#define PASS_DESCRIPTION "Lower Constant Intrinsics LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerConstantIntrinsicsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_END(LowerConstantIntrinsicsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
