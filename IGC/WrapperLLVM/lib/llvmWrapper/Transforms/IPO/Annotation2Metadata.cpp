/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/IPO/Annotation2Metadata.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/PassManager.h"
#include "llvm/IR/Value.h"
#include "llvm/Transforms/IPO.h"

#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "llvmWrapper/Transforms/IPO/Annotation2Metadata.h"

#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGCLLVM {

Annotation2MetadataLegacyPassWrapper::Annotation2MetadataLegacyPassWrapper() : ModulePass(ID) {
  initializeAnnotation2MetadataLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerModuleAnalyses(MAM);
}

bool Annotation2MetadataLegacyPassWrapper::runOnModule(llvm::Module &M) {
  if (skipModule(M))
    return false;
  // Run the New Pass Manager implementation of the pass.
  Annotation2MetadataPass Implementation;
  Implementation.run(M, MAM);
  return true;
}

char Annotation2MetadataLegacyPassWrapper::ID = 0;
ModulePass *createLegacyWrappedAnnotation2MetadataPass() { return new Annotation2MetadataLegacyPassWrapper(); }
} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "annotation2metadata-legacy-wrapped"
#define PASS_DESCRIPTION "Annotation2Metadata LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(Annotation2MetadataLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
