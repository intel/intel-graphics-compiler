/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Analysis/CallGraph.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Transforms/IPO.h"
#include "llvm/Transforms/IPO/FunctionAttrs.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/IPO/PostOrderFunctionAttrs.h"
#include "llvmWrapper/Transforms/InitializePasses.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

PostOrderFunctionAttrsLegacyPassWrapper::PostOrderFunctionAttrsLegacyPassWrapper() : ModulePass(ID) {
  initializePostOrderFunctionAttrsLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool PostOrderFunctionAttrsLegacyPassWrapper::runOnModule(llvm::Module &M) {
  // Run the New Pass Manager implementation through the CGSCC adaptor.
  CGSCCPassManager CGPM;
  CGPM.addPass(PostOrderFunctionAttrsPass());

  ModulePassManager MPM;
  MPM.addPass(createModuleToPostOrderCGSCCPassAdaptor(std::move(CGPM)));

  PreservedAnalyses PA = MPM.run(M, MAM);
  return !PA.areAllPreserved();
}

void PostOrderFunctionAttrsLegacyPassWrapper::getAnalysisUsage(AnalysisUsage &AU) const {
  // This pass only changes attributes (no IR/CFG structure changes).
  AU.setPreservesCFG();

  // Do not claim to preserve AA/etc. since attributes affect them.
  AU.addPreserved<CallGraphWrapperPass>();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
  AU.addRequired<CallGraphWrapperPass>();
}

char PostOrderFunctionAttrsLegacyPassWrapper::ID = 0;

Pass *createLegacyWrappedPostOrderFunctionAttrsPass() { return new PostOrderFunctionAttrsLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;

#define PASS_FLAG "postorder-function-attrs-legacy-wrapped"
#define PASS_DESCRIPTION "PostOrder FunctionAttrs (CGSCC) Legacy Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

IGC_INITIALIZE_PASS_BEGIN(PostOrderFunctionAttrsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                          PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(CallGraphWrapperPass)
IGC_INITIALIZE_PASS_END(PostOrderFunctionAttrsLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY,
                        PASS_ANALYSIS)
