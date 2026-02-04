/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/Transforms/Scalar/BDCE.h"

#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Scalar/BDCE.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar.h"

#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

BDCELegacyPassWrapper::BDCELegacyPassWrapper() : FunctionPass(ID) {
  initializeBDCELegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool BDCELegacyPassWrapper::runOnFunction(Function &F) {
  // The legacy pass manager implementation of the pass used to skip some functions. In the new pass manager
  // implementation this is done globally through the pass manager. Check and skip explicitly here to preserve the old
  // behavior.
  if (skipFunction(F))
    return false;

  BDCEPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void BDCELegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
#if LLVM_VERSION_MAJOR <= 16
  AU.addRequired<DemandedBitsWrapperPass>();
#endif
  AU.addPreserved<GlobalsAAWrapperPass>();
}

char BDCELegacyPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedBDCEPass() { return new BDCELegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "bdce-legacy-wrapped"
#define PASS_DESCRIPTION "Bit-Tracking Dead Code Elimination LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(BDCELegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
#if LLVM_VERSION_MAJOR <= 16
IGC_INITIALIZE_PASS_DEPENDENCY(DemandedBitsWrapperPass)
#endif
IGC_INITIALIZE_PASS_END(BDCELegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
