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
IGC_INITIALIZE_PASS_BEGIN(ResolveInlineSamplerForBindlessLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(ResolveInlineSamplerForBindlessLPM, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char ResolveInlineSamplerForBindlessLPM::ID = 0;

ResolveInlineSamplerForBindlessLPM::ResolveInlineSamplerForBindlessLPM() : FunctionPass(ID) {
  initializeResolveInlineSamplerForBindlessLPMPass(*PassRegistry::getPassRegistry());
}

bool ResolveInlineSamplerForBindless::runOnFunction(Function &F, IGC::IGCMD::MetaDataUtils *pMdUtils,
                                                    IGC::ModuleMetaData *pModMD) {
  mChanged = false;

  mMDUtils = pMdUtils;
  mModMD = pModMD;

  if (!isEntryFunc(mModMD, &F)) {
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
  ImplicitArgs ImplicitArgs(*(CI.getFunction()), mMDUtils, mModMD);

  IGC_ASSERT_MESSAGE(isa<ConstantInt>(CI.getArgOperand(0)),
                     "Sampler initializer calls can only be made with const int values!");

  auto *InlineSamplerInit = cast<ConstantInt>(CI.getArgOperand(0));
  int InlineSamplerInitValue = InlineSamplerInit->getZExtValue();
  Argument *InlineSamplerArg = ImplicitArgs.getNumberedImplicitArg(*(CI.getFunction()), ImplicitArg::INLINE_SAMPLER,
                                                                   int_cast<int>(InlineSamplerInitValue));

  IRBuilder<> Builder(&CI);
  Value *ArgToPtr = Builder.CreateIntToPtr(InlineSamplerArg, CI.getFunctionType()->getReturnType());

  CI.replaceAllUsesWith(ArgToPtr);
  CI.eraseFromParent();
}

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses ResolveInlineSamplerForBindlessNPM::run(Module &M, ModuleAnalysisManager &AM) {
  auto *pMdUtils = AM.getResult<MetaDataUtilsAnalysis>(M).MdUtils;
  auto *pModMD = AM.getResult<MetaDataUtilsAnalysis>(M).ModMD;
  ResolveInlineSamplerForBindless impl;
  bool changed = false;
  for (Function &F : M) {
    if (F.isDeclaration())
      continue;
    changed |= impl.runOnFunction(F, pMdUtils, pModMD);
  }
  return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
#endif // LLVM_VERSION_MAJOR >= 16
