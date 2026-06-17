/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm {
class Pass;
}

namespace IGC {
llvm::Pass *createRayTracingIntrinsicAnalysisPass();
llvm::Pass *createRayTracingIntrinsicResolutionPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrappers. Modeled as module passes that loop over the defined functions (the
// seeded MetaDataUtilsAnalysis is module-level; IGC passes never use skipFunction). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class RayTracingIntrinsicAnalysisNPM : public llvm::PassInfoMixin<RayTracingIntrinsicAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "raytracing-intrinsic-analysis"; }
  static bool isRequired() { return true; }
};

class RayTracingIntrinsicResolutionNPM : public llvm::PassInfoMixin<RayTracingIntrinsicResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "raytracing-intrinsic-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
llvm::Pass *createTraceRayInlinePrepPass();
llvm::Pass *createTraceRayInlineLatencySchedulerPass();
llvm::Pass *CreateTraceRayInlineLoweringPass();
llvm::Pass *createInlineRaytracing();
llvm::Pass *CreateDynamicRayManagementPass();
llvm::Pass *CreateRTGlobalsPointerLoweringPass();
llvm::Pass *createOverrideTMaxPass(unsigned OverrideValue);
} // namespace IGC
