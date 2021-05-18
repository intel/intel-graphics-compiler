/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "cmcl/Support/InitializePasses.h"
#include "cmcl/Support/TranslatorPass.h"

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

void cmcl::initializeLLVMPasses(llvm::PassRegistry &PR) {
  llvm::initializeCMCLTranslatorPass(PR);
}

extern "C" void CMCLInitializeLLVMPasses() {
  cmcl::initializeLLVMPasses(*llvm::PassRegistry::getPassRegistry());
}
