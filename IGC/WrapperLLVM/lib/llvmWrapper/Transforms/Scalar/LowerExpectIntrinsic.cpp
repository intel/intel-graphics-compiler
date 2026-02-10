/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"

#include "llvm/Transforms/Scalar/LowerExpectIntrinsic.h"
#include "llvm/Analysis/GlobalsModRef.h"
#include "llvm/Analysis/LazyBlockFrequencyInfo.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Transforms/Utils/SizeOpts.h"

#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Transforms/Scalar/LowerExpectIntrinsic.h"
#include "llvmWrapper/Transforms/InitializePasses.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;

namespace IGCLLVM {

LowerExpectIntrinsicLegacyPassWrapper::LowerExpectIntrinsicLegacyPassWrapper() : FunctionPass(ID) {
  initializeLowerExpectIntrinsicLegacyPassWrapperPass(*PassRegistry::getPassRegistry());
  PB.registerFunctionAnalyses(FAM);
}

bool LowerExpectIntrinsicLegacyPassWrapper::runOnFunction(Function &F) {
  if (skipFunction(F))
    return false;
  LowerExpectIntrinsicPass Implementation;
  Implementation.run(F, FAM);
  return true;
}

char LowerExpectIntrinsicLegacyPassWrapper::ID = 0;

FunctionPass *createLegacyWrappedLowerExpectIntrinsicPass() { return new LowerExpectIntrinsicLegacyPassWrapper(); }

} // namespace IGCLLVM

using namespace IGCLLVM;
#define PASS_FLAG "lower-expect-legacy-wrapped"
#define PASS_DESCRIPTION "Lower 'expect' Intrinsics LPM Wrapped"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS(LowerExpectIntrinsicLegacyPassWrapper, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
