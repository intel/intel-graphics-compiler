/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "SpvSubgroupBitcastShuffleResolution.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

char SpvSubgroupBitcastShuffleResolution::ID = 0;

#define PASS_FLAG "igc-spv-subgroup-bitcast-shuffle-resolution"
#define PASS_DESC "Lowering of SPV_INTEL_subgroup_bitcast_shuffle calls to intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-subgroup-bitcast-shuffle-resolution"

IGC_INITIALIZE_PASS_BEGIN(SpvSubgroupBitcastShuffleResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SpvSubgroupBitcastShuffleResolution, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

SpvSubgroupBitcastShuffleResolution::SpvSubgroupBitcastShuffleResolution() : ModulePass(ID) {
  initializeSpvSubgroupBitcastShuffleResolutionPass(*PassRegistry::getPassRegistry());
}

bool SpvSubgroupBitcastShuffleResolution::runOnModule(Module &M) {
  m_Changed = false;
  m_Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();

  visit(M);

  return m_Changed;
}

void SpvSubgroupBitcastShuffleResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  StringRef FuncName = F->getName();
  if (!FuncName.contains("__spirv_SubgroupBitcastShuffleINTEL"))
    return;

  auto ResultTy = CI.getType();
  auto DataTy = CI.getArgOperand(0)->getType();

  auto ResultBits = ResultTy->getPrimitiveSizeInBits();
  auto DataBits = DataTy->getPrimitiveSizeInBits();

  if (ResultBits == 0 || DataBits == 0) {
    m_Ctx->EmitError("__spirv_SubgroupBitcastShuffleINTEL: result and operand types must be a scalar or vector of "
                     "integer or floating point",
                     &CI);
    return;
  }
  if (ResultTy == DataTy) {
    m_Ctx->EmitError("__spirv_SubgroupBitcastShuffleINTEL: result and operand types must differ", &CI);
    return;
  }
  if (ResultBits != DataBits) {
    m_Ctx->EmitError("__spirv_SubgroupBitcastShuffleINTEL: result and operand types must have the same size", &CI);
    return;
  }

  auto *ShuffleIntrinsic = GenISAIntrinsic::getDeclaration(
      F->getParent(), GenISAIntrinsic::GenISA_SubgroupBitcastShuffle, {ResultTy, DataTy});

  IRBuilder<> Builder(&CI);
  auto *NewCI = Builder.CreateCall(ShuffleIntrinsic, CI.getArgOperand(0));
  CI.replaceAllUsesWith(NewCI);
  CI.eraseFromParent();
  m_Changed = true;
}
