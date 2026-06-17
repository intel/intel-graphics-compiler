/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/HandleDevicelibAssert/HandleDevicelibAssert.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-handle-devicelib-assert"
#define PASS_DESCRIPTION                                                                                               \
  "Remove definition of __devicelib_assert_fail if provided by DPCPP, so that IGC builtin is used."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleDevicelibAssertLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HandleDevicelibAssertLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleDevicelibAssertLPM::ID = 0;

static const char *ASSERT_FUNCTION_NAME = "__devicelib_assert_fail";

HandleDevicelibAssertLPM::HandleDevicelibAssertLPM() : ModulePass(ID) {
  initializeHandleDevicelibAssertLPMPass(*PassRegistry::getPassRegistry());
}

bool HandleDevicelibAssert::run(Module &M) {
  bool changed = false;
  for (Function &F : M) {

    if (F.getName() != ASSERT_FUNCTION_NAME)
      continue;

    if (F.isDeclaration())
      continue;

    F.deleteBody();
    changed = true;
  }
  return changed;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses HandleDevicelibAssertNPM::run(Module &M, ModuleAnalysisManager &) {
  bool changed = HandleDevicelibAssert().run(M);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
