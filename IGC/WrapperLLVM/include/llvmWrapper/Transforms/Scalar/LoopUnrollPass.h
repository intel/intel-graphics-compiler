/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/Analysis/TargetTransformInfo.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvmWrapper/ADT/Optional.h"
#include "llvmWrapper/ADT/None.h"

using namespace llvm;

namespace IGCLLVM {

struct LoopUnrollLegacyPassWrapper : public FunctionPass {
  LoopUnrollLegacyPassWrapper(int OptLevel = 2, bool OnlyWhenForced = false, bool ForgetAllSCEV = false,
                              IGCLLVM::optional<bool> AllowPartial = IGCLLVM::None,
                              IGCLLVM::optional<bool> Runtime = IGCLLVM::None,
                              IGCLLVM::optional<bool> UpperBound = IGCLLVM::None,
                              IGCLLVM::optional<bool> AllowPeeling = IGCLLVM::None,
                              IGCLLVM::optional<bool> AllowProfileBasedPeeling = IGCLLVM::None,
                              IGCLLVM::optional<unsigned> ProvidedFullUnrollMaxCount = IGCLLVM::None);
  static char ID;
  int OptLevel;
  bool OnlyWhenForced;
  bool ForgetAllSCEV;
  IGCLLVM::optional<bool> ProvidedAllowPartial;
  IGCLLVM::optional<bool> ProvidedRuntime;
  IGCLLVM::optional<bool> ProvidedUpperBound;
  IGCLLVM::optional<bool> ProvidedAllowPeeling;
  IGCLLVM::optional<bool> ProvidedAllowProfileBasedPeeling;
  IGCLLVM::optional<unsigned> ProvidedFullUnrollMaxCount;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLoopUnroll"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;

  void initializeAnalysisManagers(TargetTransformInfoWrapperPass &TTIWP);
};

Pass *createLegacyWrappedSimpleLoopUnrollPass(int OptLevel = 2, bool OnlyWhenForced = false,
                                              bool ForgetAllSCEV = false);
Pass *createLegacyWrappedLoopUnrollPass(int OptLevel = 2, bool OnlyWhenForced = false, bool ForgetAllSCEV = false,
                                        int Threshold = -1, int Count = -1, int AllowPartial = -1, int Runtime = -1,
                                        int UpperBound = -1, int AllowPeeling = -1);
} // end namespace IGCLLVM
#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H
