/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PrepareInlineSamplerForBindless.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/Optimizer/OCLBIUtils.h"

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "prepare-inline-sampler-for-bindless"
#define PASS_DESCRIPTION "Prepare OCL inline sampler for bindless"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(PrepareInlineSamplerForBindless, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(PrepareInlineSamplerForBindless, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PrepareInlineSamplerForBindless::ID = 0;

PrepareInlineSamplerForBindless::PrepareInlineSamplerForBindless() : FunctionPass(ID) {
  initializePrepareInlineSamplerForBindlessPass(*PassRegistry::getPassRegistry());
}

bool PrepareInlineSamplerForBindless::runOnFunction(Function &F) {
  mChanged = false;
  mInlineSamplerIndex = 0;

  if (!mMDUtils) {
    mMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  }

  if (!isEntryFunc(mMDUtils, &F)) {
    // Only entry functions can be assigned implicit args.
    return false;
  }

  visit(F);

  ImplicitArgs::addImageArgs(F, mArgMap, mMDUtils);
  mArgMap.clear();

  if (mChanged) {
    mMDUtils->save(F.getContext());
  }

  return mChanged;
}

void PrepareInlineSamplerForBindless::visitCallInst(CallInst &CI) {
  Function *Callee = CI.getCalledFunction();
  if (!Callee || Callee->getName() != "__bindless_sampler_initializer") {
    return;
  }

  mChanged = true;

  IGC_ASSERT_MESSAGE(isa<ConstantInt>(CI.getArgOperand(0)),
                     "Sampler initializer calls can only be made with const int values!");

  // Inline sampler doesn't associate with an explicit argument.
  // To avoid adding a new metadata entry, inline sampler value is stored as
  // explicit argument number.
  auto *InlineSamplerInit = cast<ConstantInt>(CI.getArgOperand(0));
  int InlineSamplerInitValue = InlineSamplerInit->getZExtValue();
  auto [_, IsNew] = mArgMap[ImplicitArg::INLINE_SAMPLER].insert(int_cast<int>(InlineSamplerInitValue));
  if (!IsNew) {
    // Sampler initialized by this specific value was already processed.
    // No new sampler will be created. Skip creating inline sampler metadata.
    return;
  }

  // Add metadata for the inline sampler.
  ModuleMetaData *ModMD = getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData();
  FunctionMetaData &FuncMD = ModMD->FuncMD[CI.getFunction()];
  ResourceAllocMD &ResAllocMD = FuncMD.resAllocMD;
  InlineSamplersMD InlineSamplerMD;

  CImagesBI::CreateInlineSamplerAnnotations(CI.getFunction()->getParent(), InlineSamplerMD, InlineSamplerInitValue);
  InlineSamplerMD.index = mInlineSamplerIndex++;
  ResAllocMD.inlineSamplersMD.push_back(InlineSamplerMD);
}
