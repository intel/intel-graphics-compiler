/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/WGFuncs/WGFuncResolution.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Instructions.h"
#include "GenISAIntrinsics/GenIntrinsics.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-wg-resolution"
#define PASS_DESCRIPTION "Resolve WG built-in"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(WGFuncResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(WGFuncResolutionLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char WGFuncResolutionLPM::ID = 0;

WGFuncResolutionLPM::WGFuncResolutionLPM() : ModulePass(ID) {
  initializeWGFuncResolutionLPMPass(*PassRegistry::getPassRegistry());
}

bool WGFuncResolution::run(Module &M) {
  m_changed = false;
  m_pModule = &M;

  visit(M);

  return m_changed;
}

void WGFuncResolution::visitCallInst(CallInst &callInst) {
  Function *pCalledFunc = callInst.getCalledFunction();
  if (!pCalledFunc) {
    // Indirect call
    return;
  }
  StringRef funcName = pCalledFunc->getName();
  if (IGCLLVM::starts_with(funcName, "__builtin_IB_work_group_any")) {
    SmallVector<Value *, 1> args;

    args.push_back(callInst.getOperand(0));

    Function *isaIntrinFunc = GenISAIntrinsic::getDeclaration(m_pModule, GenISAIntrinsic::GenISA_WorkGroupAny);
    CallInst *isaIntrinCall =
        CallInst::Create(isaIntrinFunc, args, callInst.getName(), IGCLLVM::insertPosition(&callInst));

    isaIntrinCall->setDebugLoc(callInst.getDebugLoc());

    callInst.replaceAllUsesWith(isaIntrinCall);
    callInst.eraseFromParent();
    m_changed = true;
  }
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses WGFuncResolutionNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = WGFuncResolution().run(M);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
