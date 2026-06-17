/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/OpenCLPasses/BreakdownIntrinsic/BreakdownIntrinsic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

// Register pass to igc-opt
#define PASS_FLAG "breakdown-intrinsics"
#define PASS_DESCRIPTION "Breakdown intrinsics into simpler operations to enable better optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(BreakdownIntrinsicPassLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(BreakdownIntrinsicPassLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char BreakdownIntrinsicPassLPM::ID = 0;

BreakdownIntrinsicPassLPM::BreakdownIntrinsicPassLPM() : FunctionPass(ID) {
  initializeBreakdownIntrinsicPassLPMPass(*PassRegistry::getPassRegistry());
}

void BreakdownIntrinsicPass::visitIntrinsicInst(llvm::IntrinsicInst &I) {
  ModuleMetaData &modMD = *this->modMD;
  llvm::IRBuilder<> builder(&I);
  bool md_added = false;

  auto pCtx = m_pCtx;

  if (I.getIntrinsicID() == llvm::Intrinsic::fmuladd ||
      // For FMA only break it up if unsafe math optimizations are set
      (I.getIntrinsicID() == llvm::Intrinsic::fma && modMD.compOpt.UnsafeMathOptimizations)) {
    llvm::Value *pMulInst = builder.CreateFMul(I.getOperand(0), I.getOperand(1));
    llvm::Value *pAddInst = builder.CreateFAdd(pMulInst, I.getOperand(2));
    I.replaceAllUsesWith(pAddInst);
    I.eraseFromParent();
    m_changed = true;

    // The presence of fmuladd indicates that the fp_contract needs to be set.
    if (pCtx->m_DriverInfo.NeedsBreakdownMulAdd() && !md_added) {
      modMD.compOpt.MadEnable = true;
      md_added = true;
    }
  }
}

bool BreakdownIntrinsicPass::runOnFunction(llvm::Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils,
                                           IGC::ModuleMetaData *pModMD, IGC::CodeGenContext *pCtx) {
  m_pMdUtils = pMdUtils;
  modMD = pModMD;
  m_pCtx = pCtx;
  visit(F);
  return m_changed;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses BreakdownIntrinsicPassNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto &MDU = AM.getResult<MetaDataUtilsAnalysis>(M);
  CodeGenContext *pCtx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
  BreakdownIntrinsicPass impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, MDU.MdUtils, MDU.ModMD, pCtx);
  }
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
