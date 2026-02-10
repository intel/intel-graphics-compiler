/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "MSAAInsertDiscard.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMUtils.h"
#include "Compiler/CodeGenPublic.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-MSAAInsertDiscard"
#define PASS_DESCRIPTION "Insert discard code for MSAA MSC kernels"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(MSAAInsertDiscard, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(MSAAInsertDiscard, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char MSAAInsertDiscard::ID = 0;

MSAAInsertDiscard::MSAAInsertDiscard() : FunctionPass(ID) {
  done = false;
  m_kernelSize = IGC_GET_FLAG_VALUE(MSAAClearedKernel);
  initializeWorkaroundAnalysisPass(*PassRegistry::getPassRegistry());
};

int MSAAInsertDiscard::getiCMPValue() {
  switch (m_kernelSize) {
  case 2:
    return 3;

  case 4:
    return 255;

  case 8:
  case 16:
    return -1;

  default:
    return 0;
  }
}

void MSAAInsertDiscard::visitCallInst(CallInst &I) {
  if (done)
    return;

  if (const GenIntrinsicInst *intr = dyn_cast<GenIntrinsicInst>(&I)) {
    GenISAIntrinsic::ID intrID = intr->getIntrinsicID();
    if (intrID != GenISAIntrinsic::GenISA_ldmsptr)
      return;

    // For the case of m_kernelSize is 2
    // % 6 = call <2 x i32> @llvm.genx.GenISA.ldmcsptr.v2i32.i32.p196608v4f32(
    //         i32 % 4, i32 % 5, i32 0, i32 0,
    //         <4 x float> addrspace(196608) * null, i32 0, i32 0, i32 0)
    // % 7 = extractelement <2 x i32> % 6, i32 0
    // % 8 = icmp eq i32 % 7, 3                     <------- inserted ------->
    // call void @llvm.genx.GenISA.discard(i1 % 8)  <------- inserted ------->
    // % 9 = extractelement <2 x i32> % 6, i32 1
    // % 10 = call fast <4 x float> @llvm.genx.GenISA.ldmsptr.v4f32.p196608v4f32(
    //          i32 0, i32 % 7, i32 % 9, i32 % 4, i32 % 5, i32 0, i32 0,
    //          <4 x float> addrspace(196608) * null, i32 0, i32 0, i32 0)

    [[maybe_unused]] GenIntrinsicInst *ldmcs = nullptr;
    Value *extractData1 = I.getOperand(1);
    Value *extractData2 = I.getOperand(2);
    Value *mcsData = nullptr;
    Value *iCmpEq = nullptr;
    if (ExtractElementInst *extractInst1 = dyn_cast<ExtractElementInst>(extractData1)) {
      mcsData = extractInst1->getOperand(0);
      ldmcs = dyn_cast<GenIntrinsicInst>(mcsData);

      IGC_ASSERT(ldmcs != NULL);
      if (m_kernelSize == 16) {
        // insert before current ldmsptr
        m_builder->SetInsertPoint(&I);
        Value *andValue = m_builder->CreateAnd(extractData1, extractData2);
        iCmpEq = m_builder->CreateICmpEQ(andValue, m_builder->getInt32(getiCMPValue()));
      } else {
        // insert before the 2nd extractelement
        m_builder->SetInsertPoint(dyn_cast<ExtractElementInst>(extractData2));
        iCmpEq = m_builder->CreateICmpEQ(extractData1, m_builder->getInt32(getiCMPValue()));
      }

      Function *func_discard = llvm::GenISAIntrinsic::getDeclaration(I.getParent()->getParent()->getParent(),
                                                                     GenISAIntrinsic::GenISA_discard);
      m_builder->CreateCall(func_discard, {iCmpEq});

      done = true;
    }
  }
}

bool MSAAInsertDiscard::runOnFunction(Function &F) {
  m_pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
  CodeGenContext *cgCtx = m_pCtxWrapper->getCodeGenContext();
  IRBuilder<> builder(F.getContext());
  m_builder = &builder;
  visit(F);
  DumpLLVMIR(cgCtx, "AfterMSAAInsertDiscard");
  return true;
}
