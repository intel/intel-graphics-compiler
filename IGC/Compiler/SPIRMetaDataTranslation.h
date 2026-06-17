/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
bool isLegalOCLVersion(int major, int minor);

// Shared implementation. Translates SPIR metadata to IGC metadata. Used by both
// the legacy and the new-pass-manager wrappers below; it is not itself an
// llvm::Pass. The context is passed in explicitly so it works with either pass
// manager.
class SPIRMetaDataTranslation {
public:
  SPIRMetaDataTranslation() {}
  ~SPIRMetaDataTranslation() {}

  static llvm::StringRef getPassName() { return "SPIR to IGC metadata translator"; }

  void translateKernelMetadataIntoOpenCLKernelsMD(llvm::Module &M);
  bool run(llvm::Module &M, CodeGenContext *pCtx, IGCMD::MetaDataUtils *pIgcMDUtils, ModuleMetaData *modMD);

private:
  // Set by run(); used by the body in place of getAnalysis<>.
  CodeGenContext *m_pCtx = nullptr;
};

// Legacy Pass Manager wrapper.
class SPIRMetaDataTranslationLPM : public llvm::ModulePass {
public:
  static char ID;

  SPIRMetaDataTranslationLPM();
  ~SPIRMetaDataTranslationLPM() {}

  llvm::StringRef getPassName() const override { return SPIRMetaDataTranslation::getPassName(); }

  bool runOnModule(llvm::Module &M) override;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper.
class SPIRMetaDataTranslationNPM : public llvm::PassInfoMixin<SPIRMetaDataTranslationNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-spir-metadata-translation"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
