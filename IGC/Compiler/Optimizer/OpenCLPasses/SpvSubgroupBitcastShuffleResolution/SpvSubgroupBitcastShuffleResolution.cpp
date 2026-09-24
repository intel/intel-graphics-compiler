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

char SpvSubgroupBitcastShuffleResolutionLPM::ID = 0;

#define PASS_FLAG "igc-spv-subgroup-bitcast-shuffle-resolution"
#define PASS_DESC "Lowering of SPV_INTEL_subgroup_bitcast_shuffle calls to intrinsics"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
#define DEBUG_TYPE "spv-subgroup-bitcast-shuffle-resolution"

IGC_INITIALIZE_PASS_BEGIN(SpvSubgroupBitcastShuffleResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(SpvSubgroupBitcastShuffleResolutionLPM, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)

SpvSubgroupBitcastShuffleResolutionLPM::SpvSubgroupBitcastShuffleResolutionLPM() : ModulePass(ID) {
  initializeSpvSubgroupBitcastShuffleResolutionLPMPass(*PassRegistry::getPassRegistry());
}

bool SpvSubgroupBitcastShuffleResolution::run(Module &M, CodeGenContext *pCtx) {
  m_Changed = false;
  m_Ctx = pCtx;

  visit(M);

  return m_Changed;
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses SpvSubgroupBitcastShuffleResolutionNPM::run(Module &M, ModuleAnalysisManager &AM) {
  bool changed = SpvSubgroupBitcastShuffleResolution().run(M, AM.getResult<CodeGenContextAnalysis>(M).Ctx);
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16

void SpvSubgroupBitcastShuffleResolution::visitCallInst(CallInst &CI) {
  Function *F = CI.getCalledFunction();
  if (!F)
    return;

  StringRef FuncName = F->getName();
  if (!FuncName.contains("__spirv_SubgroupBitcastShuffleINTEL"))
    return;

  auto ResultTy = CI.getType();
  auto DataTy = CI.getArgOperand(0)->getType();

  // The Supported Types table restricts SPV_INTEL_subgroup_bitcast_shuffle to
  // integers despite the instruction's broader numerical-type description.
  auto IsSupportedType = [](Type *Ty) {
    Type *ScalarTy = Ty->getScalarType();
    if (!ScalarTy->isIntegerTy(8) && !ScalarTy->isIntegerTy(16) && !ScalarTy->isIntegerTy(32) &&
        !ScalarTy->isIntegerTy(64))
      return false;

    if (auto *VecTy = dyn_cast<FixedVectorType>(Ty)) {
      unsigned NumElements = VecTy->getNumElements();
      return NumElements == 2 || NumElements == 4 || NumElements == 8 || NumElements == 16;
    }
    return Ty->isIntegerTy();
  };

  if (!IsSupportedType(ResultTy) || !IsSupportedType(DataTy)) {
    m_Ctx->EmitError("__spirv_SubgroupBitcastShuffleINTEL: result and operand types must be 8-, 16-, 32-, or 64-bit "
                     "integers or vectors of 2, 4, 8, or 16 such integers",
                     &CI);
    return;
  }
  if (ResultTy == DataTy) {
    m_Ctx->EmitError("__spirv_SubgroupBitcastShuffleINTEL: result and operand types must differ", &CI);
    return;
  }
  if (ResultTy->getPrimitiveSizeInBits() != DataTy->getPrimitiveSizeInBits()) {
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
