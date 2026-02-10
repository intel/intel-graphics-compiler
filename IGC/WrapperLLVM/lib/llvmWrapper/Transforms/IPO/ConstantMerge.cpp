/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/ConstantMerge.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/ConstantMerge.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

ConstantMergeLegacyPassWrapper::ConstantMergeLegacyPassWrapper() : ModulePass(ID) {
  initializeConstantMergeLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
  PB.registerCGSCCAnalyses(CGAM);
  PB.registerFunctionAnalyses(FAM);
  PB.registerLoopAnalyses(LAM);
  PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);
}

bool ConstantMergeLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  ConstantMergePass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char ConstantMergeLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedConstantMergePass() { return new ConstantMergeLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "constmerge-legacy-wrapped"
#define PASS_DESCRIPTION "Merge Duplicate Global Constants LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(ConstantMergeLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
