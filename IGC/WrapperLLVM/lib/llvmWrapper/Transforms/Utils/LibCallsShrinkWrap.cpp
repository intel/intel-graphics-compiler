/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Utils/LibCallsShrinkWrap.h"
#include "llvm/Analysis/AssumptionCache.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/MemorySSAUpdater.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/Transforms/Scalar.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Utils/LibCallsShrinkWrap.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LibCallsShrinkWrapLegacyPassWrapper::LibCallsShrinkWrapLegacyPassWrapper() : FunctionPass(ID) {
  initializeLibCallsShrinkWrapLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool LibCallsShrinkWrapLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  // Run the New Pass Manager implementation of the pass.
  LibCallsShrinkWrapPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

void LibCallsShrinkWrapLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addPreserved<DominatorTreeWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
}

char LibCallsShrinkWrapLegacyPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedLibCallsShrinkWrapPass() { return new LibCallsShrinkWrapLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "libcalls-shrinkwrap-legacy-wrapped"
#define PASS_DESCRIPTION "Conditionally eliminate dead library calls LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LibCallsShrinkWrapLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(LibCallsShrinkWrapLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
