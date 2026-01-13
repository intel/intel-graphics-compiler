/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H
#define IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H

#include "llvm/Analysis/LoopAnalysisManager.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/IR/PassManager.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include <optional>
using namespace llvm;

namespace IGCLLVM {

struct LoopUnrollLegacyPassWrapper : public FunctionPass {
  LoopUnrollLegacyPassWrapper(int OptLevel = 2, bool OnlyWhenForced = false, bool ForgetAllSCEV = false,
                              std::optional<unsigned> Threshold = std::nullopt,
                              std::optional<unsigned> Count = std::nullopt,
                              std::optional<bool> AllowPartial = std::nullopt,
                              std::optional<bool> Runtime = std::nullopt, std::optional<bool> UpperBound = std::nullopt,
                              std::optional<bool> AllowPeeling = std::nullopt,
                              std::optional<bool> AllowProfileBasedPeeling = std::nullopt,
                              std::optional<unsigned> ProvidedFullUnrollMaxCount = std::nullopt);
  static char ID;
  int OptLevel;
  bool OnlyWhenForced;
  bool ForgetAllSCEV;
  std::optional<unsigned> ProvidedCount;
  std::optional<unsigned> ProvidedThreshold;
  std::optional<bool> ProvidedAllowPartial;
  std::optional<bool> ProvidedRuntime;
  std::optional<bool> ProvidedUpperBound;
  std::optional<bool> ProvidedAllowPeeling;
  std::optional<bool> ProvidedAllowProfileBasedPeeling;
  std::optional<unsigned> ProvidedFullUnrollMaxCount;

  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  llvm::StringRef getPassName() const override { return "LegacyWrappedLoopUnroll"; }

private:
  LoopAnalysisManager LAM;
  FunctionAnalysisManager FAM;
  CGSCCAnalysisManager CGAM;
  ModuleAnalysisManager MAM;
  PassBuilder PB;
};

Pass *createLegacyWrappedSimpleLoopUnrollPass(int OptLevel = 2, bool OnlyWhenForced = false,
                                              bool ForgetAllSCEV = false);
Pass *createLegacyWrappedLoopUnrollPass(int OptLevel = 2, bool OnlyWhenForced = false, bool ForgetAllSCEV = false,
                                        int Threshold = -1, int Count = -1, int AllowPartial = -1, int Runtime = -1,
                                        int UpperBound = -1, int AllowPeeling = -1);
} // end namespace IGCLLVM
#endif // IGCLLVM_TRANSFORMS_SCALAR_LEGACY_LOOPUNROLL_H