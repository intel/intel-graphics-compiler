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
#define PASS_DESCRIPTION "Remove definition of __devicelib_assert_fail if provided by DPCPP, so that IGC builtin is used."
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HandleDevicelibAssert, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HandleDevicelibAssert, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HandleDevicelibAssert::ID = 0;

static const char *ASSERT_FUNCTION_NAME = "__devicelib_assert_fail";

HandleDevicelibAssert::HandleDevicelibAssert()
    : ModulePass(ID) {
  initializeHandleDevicelibAssertPass(
      *PassRegistry::getPassRegistry());
}

bool HandleDevicelibAssert::runOnModule(Module &M) {
  bool changed = false;
  for (Function &F : M) {

    if (!F.getName().equals(ASSERT_FUNCTION_NAME))
      continue;

    if (F.isDeclaration()) continue;

    F.deleteBody();
    changed = true;

  }
  return changed;
}
