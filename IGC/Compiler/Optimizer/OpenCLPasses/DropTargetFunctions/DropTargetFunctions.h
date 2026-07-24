/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class DropTargetFunctions final {
public:
  DropTargetFunctions();
  ~DropTargetFunctions() {}

  static llvm::StringRef getPassName() { return "DropTargetFunctions"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *MdUtils, IGC::ModuleMetaData *ModMD);

private:
  bool VerboseLog = false;
};

// Legacy Pass Manager wrapper.
class DropTargetFunctionsLPM final : public llvm::ModulePass {
public:
  static char ID;

  DropTargetFunctionsLPM();
  ~DropTargetFunctionsLPM() {}

  llvm::StringRef getPassName() const override { return DropTargetFunctions::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  bool runOnModule(llvm::Module &M) override {
    return DropTargetFunctions().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                     getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class DropTargetFunctionsNPM : public llvm::PassInfoMixin<DropTargetFunctionsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-drop-target-fns"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
}; // namespace IGC
