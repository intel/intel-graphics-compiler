/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/Scalar/MergedLoadStoreMotion.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/MergedLoadStoreMotion.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

MergedLoadStoreMotionLegacyPassWrapper::MergedLoadStoreMotionLegacyPassWrapper() : FunctionPass(ID) {
  initializeMergedLoadStoreMotionLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool MergedLoadStoreMotionLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  MergedLoadStoreMotionPass Pass;
  PreservedAnalyses PA = Pass.run(F, FAM);
  return !PA.areAllPreserved();
}

void MergedLoadStoreMotionLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<AAResultsWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
}

char MergedLoadStoreMotionLegacyPassWrapper::ID = 0;

FunctionPass *createLegacyWrappedMergedLoadStoreMotionPass() { return new MergedLoadStoreMotionLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "merged-load-store-motion-legacy-wrapped"
#define PASS_DESCRIPTION "Merge Load/Stores in Diamonds LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(MergedLoadStoreMotionLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_END(MergedLoadStoreMotionLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
