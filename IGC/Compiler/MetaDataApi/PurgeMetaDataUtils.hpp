/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PurgeMetaDataUtils {
public:
  PurgeMetaDataUtils() {}
  ~PurgeMetaDataUtils() {}

  static llvm::StringRef getPassName() { return "PurgeMetaDataUtilsPass"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD);
};

// Legacy Pass Manager wrapper.
class PurgeMetaDataUtilsLPM : public llvm::ModulePass {
public:
  PurgeMetaDataUtilsLPM();

  ~PurgeMetaDataUtilsLPM() {}

  virtual llvm::StringRef getPassName() const override { return PurgeMetaDataUtils::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<MetaDataUtilsWrapper>(); }

  // Pass identification, replacement for typeid
  static char ID;

private:
  PurgeMetaDataUtils m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class PurgeMetaDataUtilsNPM : public llvm::PassInfoMixin<PurgeMetaDataUtilsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-purgeMetaDataUtils-import"; }
  static bool isRequired() { return true; }

private:
  PurgeMetaDataUtils m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

bool purgeMetaDataUtils(llvm::Module &M, MetaDataUtilsWrapper *MDUW);
bool purgeMetaDataUtils(llvm::Module &M, IGCMD::MetaDataUtils *MDUtils, ModuleMetaData *ModMD);
} // namespace IGC
