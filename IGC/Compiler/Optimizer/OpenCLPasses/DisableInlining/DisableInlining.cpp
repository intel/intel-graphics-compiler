/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/DisableInlining/DisableInlining.h"

#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-disable-inlining"
#define PASS_DESCRIPTION "Disable function inlining."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(DisableInlining, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(DisableInlining, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char DisableInlining::ID = 0;

DisableInlining::DisableInlining() : FunctionPass(ID) {
  initializeDisableInliningPass(*PassRegistry::getPassRegistry());
}

bool DisableInlining::runOnFunction(llvm::Function &F) {
  if (F.isDeclaration() || F.hasFnAttribute(llvm::Attribute::NoInline)) {
    return false;
  }
  if (F.hasFnAttribute(llvm::Attribute::AlwaysInline)) {
    F.removeFnAttr(llvm::Attribute::AlwaysInline);
  }
  F.addFnAttr(llvm::Attribute::NoInline);

  for (auto &I : llvm::instructions(F)) {
    if (auto *CI = dyn_cast<CallInst>(&I)) {
      if (CI->hasFnAttr(llvm::Attribute::AlwaysInline)) {
        CI->removeFnAttr(llvm::Attribute::AlwaysInline);
      }
    }
  }

  return true;
}