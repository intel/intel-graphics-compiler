/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "llvm/IR/Instructions.h"
#include <llvm/IR/Function.h>
#include "llvm/IR/Module.h"
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/AssumptionCache.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// forward decl
namespace IGCMD {
class MetaDataUtils;
}

//
// CodeAssumption inserts llvm.assume to make sure some of code's
// attributes holds. For example, OCL's get_global_id() will be
// always positive, so we insert llvm.assume for its return value.
// This llvm.assume will help value tracking (verifying whether an
// value is positive or not). Currently, value tracking is used
// by StatelessToStateful optimization.
//
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class CodeAssumption {
public:
  CodeAssumption() {}
  ~CodeAssumption() {}

  static llvm::StringRef getPassName() { return "CodeAssumption"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD);

  // APIs used directly
  static bool addAssumption(llvm::Function *F, llvm::AssumptionCache *AC);

  static bool IsSGIdUniform(IGCMD::MetaDataUtils *pMDU, ModuleMetaData *modMD, llvm::Function *F);

private:
  bool m_changed{};

  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;
  ModuleMetaData *m_modMD = nullptr;

  // Simple change to help uniform analysis (later).
  void uniformHelper(llvm::Module *M);

  // Add llvm.assume to assist other optimization such statelessToStateful
  void addAssumption(llvm::Module *M);

  // helpers
  static bool isPositiveIndVar(llvm::PHINode *PN, const llvm::DataLayout *DL, llvm::AssumptionCache *AC);
};

// Legacy Pass Manager wrapper.
class CodeAssumptionLPM : public llvm::ModulePass {
public:
  static char ID;

  CodeAssumptionLPM() : ModulePass(ID) {}

  llvm::StringRef getPassName() const override { return CodeAssumption::getPassName(); }

  bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

private:
  CodeAssumption m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class CodeAssumptionNPM : public llvm::PassInfoMixin<CodeAssumptionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-codeassumption"; }
  static bool isRequired() { return true; }

private:
  CodeAssumption m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
