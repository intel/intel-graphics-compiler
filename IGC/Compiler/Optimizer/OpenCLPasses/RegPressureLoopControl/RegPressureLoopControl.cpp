/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/RegPressureLoopControl/RegPressureLoopControl.hpp"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"


using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "igc-register-pressure-loop-control"
#define PASS_DESCRIPTION "Controls the register pressure for given loop"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RegPressureLoopControl, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterPressureEstimate)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(RegPressureLoopControl, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char RegPressureLoopControl::ID = 0;

const int RegPressureLoopControl::MaxRegPressureLoopHdr = 2000;

RegPressureLoopControl::RegPressureLoopControl() : llvm::FunctionPass(ID) {
  initializeRegPressureLoopControlPass(*PassRegistry::getPassRegistry());
}

bool RegPressureLoopControl::runOnFunction(llvm::Function &pFunc) {
  LoopInfo &LI = getAnalysis<LoopInfoWrapperPass>().getLoopInfo();
  m_pRegisterPressureEstimate = &getAnalysis<RegisterPressureEstimate>();
  IGC::CodeGenContext *pCtx =
      getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  pCtx->m_FuncHasExpensiveLoops[&pFunc] = false;

  for (auto LI_i : LI) {
    llvm::Loop *L = LI_i;

    IGC_ASSERT(nullptr != m_pRegisterPressureEstimate);
    // if no live range info
    if (!m_pRegisterPressureEstimate->isAvailable()) {
      continue;
    }
    m_pRegisterPressureEstimate->buildRPMapPerInstruction();

    llvm::BasicBlock *Header = L->getLoopPreheader();

    int hdrRegPressure =
        m_pRegisterPressureEstimate->getMaxRegisterPressure(Header);

    if (hdrRegPressure > MaxRegPressureLoopHdr) {
      pCtx->m_FuncHasExpensiveLoops[&pFunc] = true;
      return false;
    }
  }

  return false;
}