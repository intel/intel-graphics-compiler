/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_IPO_LEGACY_PASSMANAGERBUILDER_H
#define IGCLLVM_TRANSFORMS_IPO_LEGACY_PASSMANAGERBUILDER_H

#include "IGC/common/LLVMWarningsPush.hpp"
#if LLVM_VERSION_MAJOR < 16
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#endif
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "IGC/common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 16
class PassManagerBuilder : public llvm::PassManagerBuilder {};
#else
class PassManagerBuilder {
private:
  void addInitialAliasAnalysisPasses(legacy::PassManagerBase &PM) const;
  void addFunctionSimplificationPasses(legacy::PassManagerBase &MPM);
  void addVectorPasses(legacy::PassManagerBase &PM, bool IsFullLTO);

public:
  PassManagerBuilder();
  ~PassManagerBuilder();
  PassManagerBuilder(const PassManagerBuilder &) = delete;
  PassManagerBuilder &operator=(const PassManagerBuilder &) = delete;

  unsigned OptLevel;
  unsigned SizeLevel;
  Pass *Inliner;
  bool DisableUnrollLoops;
  bool LoopVectorize;
  bool SLPVectorize;
  unsigned LicmMssaOptCap;
  unsigned LicmMssaNoAccForPromotionCap;
  bool ForgetAllSCEVInLoopUnroll;
  void populateFunctionPassManager(legacy::FunctionPassManager &FPM);
  void populateModulePassManager(legacy::PassManagerBase &MPM);
};
#endif
} // namespace IGCLLVM

#endif
