/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/Optimizer/RuntimeValueVectorExtractPass.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/DerivedTypes.h>

using namespace llvm;
using namespace IGC;

#define PASS_FLAG "igc-runtimevalue-vector-extract-pass"
#define PASS_DESCRIPTION "Shader extract element from vector of constants optimization"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(RuntimeValueVectorExtractPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(RuntimeValueVectorExtractPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

namespace IGC {

char RuntimeValueVectorExtractPass::ID = 0;

////////////////////////////////////////////////////////////////////////////
RuntimeValueVectorExtractPass::RuntimeValueVectorExtractPass() : llvm::FunctionPass(ID), changed(false) {
  initializeRuntimeValueVectorExtractPassPass(*llvm::PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////
void RuntimeValueVectorExtractPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const { AU.setPreservesCFG(); }

////////////////////////////////////////////////////////////////////////////
bool RuntimeValueVectorExtractPass::runOnFunction(llvm::Function &F) {
  changed = false;
  visit(F);
  return changed;
}

////////////////////////////////////////////////////////////////////////////
// @brief Converts extracts of elements from corresponding RuntimeValue
// vector to RuntimeValue calls returning concrete scalars.
// Only extracts using constant indexes are converted.
// Only 32-bit and 64-bit RuntimeValues are supported at the moment.
//
// Replace:
//   %0 = call <8 x i32> @llvm.genx.GenISA.RuntimeValue.v8i32(i32 4)
//   %scalar  = extractelement <8 x i32> %0, i32 0
//   %scalar1 = extractelement <8 x i32> %0, i32 1
// with:
//   %scalar  = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 4)
//   %scalar1 = call i32 @llvm.genx.GenISA.RuntimeValue.i32(i32 5)
void RuntimeValueVectorExtractPass::visitExtractElementInst(llvm::ExtractElementInst &I) {
  // Optimization works only on constant indexes
  if (isa<ConstantInt>(I.getIndexOperand())) {
    GenIntrinsicInst *GII = dyn_cast<GenIntrinsicInst>(I.getVectorOperand());
    if (GII && GII->getIntrinsicID() == GenISAIntrinsic::GenISA_RuntimeValue && isa<ConstantInt>(GII->getOperand(0)) &&
        isa<IGCLLVM::FixedVectorType>(GII->getType())) {
      IGCLLVM::FixedVectorType *giiVectorType = cast<IGCLLVM::FixedVectorType>(GII->getType());
      // Only 32-bit and 64-bit values are supported at the moment
      if (giiVectorType->getElementType()->getPrimitiveSizeInBits() == 32 ||
          giiVectorType->getElementType()->getPrimitiveSizeInBits() == 64) {
        bool is64bit = giiVectorType->getElementType()->getPrimitiveSizeInBits() == 64;

        IRBuilder<> Builder(&I);
        Function *runtimeValueFunc = GenISAIntrinsic::getDeclaration(
            I.getModule(), GenISAIntrinsic::GenISA_RuntimeValue, giiVectorType->getElementType());

        const uint32_t eeiIndex = int_cast<uint32_t>(cast<ConstantInt>(I.getIndexOperand())->getZExtValue());
        const uint32_t giiOffset = int_cast<uint32_t>(cast<ConstantInt>(GII->getOperand(0))->getZExtValue());

        // Calculate new offset
        const uint32_t offset = giiOffset + (is64bit ? eeiIndex * 2 : eeiIndex);

        Value *CI = Builder.CreateCall(runtimeValueFunc, Builder.getInt32(offset));

        I.replaceAllUsesWith(CI);
        I.eraseFromParent();

        changed = true;
      } else {
        IGC_ASSERT_MESSAGE(0, "Only 32-bit and 64-bit values are supported at the moment");
      }
    }
  }
}

} // namespace IGC
