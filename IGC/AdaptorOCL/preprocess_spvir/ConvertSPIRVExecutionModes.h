/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"

namespace IGC {
// Translates entry-point execution modes recorded by the SPIR-V reader in the
// 'spirv.ExecutionMode' named metadata into per-function attributes, which -
// unlike metadata - survive module linking.
//
// Currently handles SPV_INTEL_maximum_registers:
//   MaximumRegistersINTEL (6461), MaximumRegistersIdINTEL (6462) and
//   NamedMaximumRegistersINTEL (6463)
// all of which are folded into the "num-grf-per-thread" attribute, holding the
// legalized register count or "0" for AutoINTEL. See
// https://github.khronos.org/SPIRV-Registry/extensions/INTEL/SPV_INTEL_maximum_registers.html
class ConvertSPIRVExecutionModes {
public:
  ConvertSPIRVExecutionModes() {}
  ~ConvertSPIRVExecutionModes() {}

  static llvm::StringRef getPassName() { return "ConvertSPIRVExecutionModes"; }

  bool run(llvm::Module &M, CodeGenContext *Ctx, ModuleMetaData *MD);

private:
  bool handleMaximumRegisters(llvm::Function *F, uint32_t ExecutionMode, const llvm::Metadata *Value);
  bool dropNumThreadPerEUAnnotation(llvm::Function *F);

  CodeGenContext *Context = nullptr;
  ModuleMetaData *Metadata = nullptr;
  bool MetadataChanged = false;
};

class ConvertSPIRVExecutionModesLPM : public llvm::ModulePass {
public:
  static char ID;

  ConvertSPIRVExecutionModesLPM();
  ~ConvertSPIRVExecutionModesLPM() {}

  llvm::StringRef getPassName() const override { return ConvertSPIRVExecutionModes::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;
};

#if LLVM_VERSION_MAJOR >= 16
class ConvertSPIRVExecutionModesNPM : public llvm::PassInfoMixin<ConvertSPIRVExecutionModesNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-convert-spirv-execution-modes"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
