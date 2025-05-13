/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "ResolveInlineSamplerForBindless.hpp"

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace IGC;
using namespace llvm;

// Register pass to igc-opt
#define PASS_FLAG "resolve-inline-sampler-for-bindless"
#define PASS_DESCRIPTION "Resolve OCL inline sampler for bindless"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(ResolveInlineSamplerForBindless, PASS_FLAG,
                          PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResolveInlineSamplerForBindless, PASS_FLAG,
                        PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveInlineSamplerForBindless::ID = 0;

ResolveInlineSamplerForBindless::ResolveInlineSamplerForBindless()
    : FunctionPass(ID) {
  initializeResolveInlineSamplerForBindlessPass(
      *PassRegistry::getPassRegistry());
}

bool ResolveInlineSamplerForBindless::runOnFunction(Function &F) {
  mChanged = false;

  if (!mMDUtils) {
    mMDUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  }

  if (!isEntryFunc(mMDUtils, &F)) {
    // Only entry functions can be assigned implicit args.
    return false;
  }

  visit(F);

  return mChanged;
}

void ResolveInlineSamplerForBindless::visitCallInst(CallInst &CI) {
  Function *Callee = CI.getCalledFunction();
  if (!Callee || Callee->getName() != "__bindless_sampler_initializer") {
    return;
  }

  mChanged = true;

  // Bindless inline sampler is passed via implicit kernel argument.
  ImplicitArgs ImplicitArgs(*(CI.getFunction()), mMDUtils);

  IGC_ASSERT_MESSAGE(isa<ConstantInt>(CI.getArgOperand(0)),
      "Sampler initializer calls can only be made with const int values!");

  auto *InlineSamplerInit = cast<ConstantInt>(CI.getArgOperand(0));
  int InlineSamplerInitValue = InlineSamplerInit->getZExtValue();
  Argument *InlineSamplerArg = ImplicitArgs.getNumberedImplicitArg(
      *(CI.getFunction()), ImplicitArg::INLINE_SAMPLER,
      int_cast<int>(InlineSamplerInitValue));

  IRBuilder<> Builder(&CI);
  Value *ArgToPtr = Builder.CreateIntToPtr(
      InlineSamplerArg, CI.getFunctionType()->getReturnType());

  CI.replaceAllUsesWith(ArgToPtr);
  CI.eraseFromParent();
}
