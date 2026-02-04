/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Utils/InjectTLIMappings.h"
#include "llvmWrapper/Analysis/DemandedBits.h"
#include "llvmWrapper/Analysis/LoopAccessAnalysis.h"
#include "llvm/Analysis/DemandedBits.h"
#include "llvm/Analysis/LoopAccessAnalysis.h"

#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/OptimizationRemarkEmitter.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/VectorUtils.h"
#include "llvm/Transforms/Scalar.h"

#include "llvmWrapper/Transforms/Utils/InjectTLIMappings.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

InjectTLIMappingsLegacyPassWrapper::InjectTLIMappingsLegacyPassWrapper() : FunctionPass(ID) {
  initializeInjectTLIMappingsLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool InjectTLIMappingsLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;

  // Run the New Pass Manager implementation of the pass.
  InjectTLIMappings Implementation;
  Implementation.run(F, FAM);
  return true;
}

void InjectTLIMappingsLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addPreserved<TargetLibraryInfoWrapperPass>();
  AU.addPreserved<ScalarEvolutionWrapperPass>();
  AU.addPreserved<AAResultsWrapperPass>();
#if LLVM_VERSION_MAJOR <= 16
  AU.addPreserved<LoopAccessLegacyAnalysis>();
  AU.addPreserved<DemandedBitsWrapperPass>();
#endif
  AU.addPreserved<OptimizationRemarkEmitterWrapperPass>();
  AU.addPreserved<GlobalsAAWrapperPass>();
}

char InjectTLIMappingsLegacyPassWrapper::ID = 0;
FunctionPass *createLegacyWrappedInjectTLIMappingsPass() { return new InjectTLIMappingsLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "inject-tli-mappings-legacy-wrapped"
#define PASS_DESCRIPTION "Inject TLI Mappings LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(InjectTLIMappingsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_END(InjectTLIMappingsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)