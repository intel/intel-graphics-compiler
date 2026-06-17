/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class AggregateArgumentsAnalysis {
public:
  AggregateArgumentsAnalysis() {}

  static llvm::StringRef getPassName() { return "AggregateArgumentsAnalysis"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);

private:
  void addImplictArgs(llvm::Type *type, uint64_t baseAllocaOffset);

private:
  const llvm::DataLayout *m_pDL{};
  ImplicitArg::StructArgList m_argList;
  IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
};

// Legacy Pass Manager wrapper.
class AggregateArgumentsAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  AggregateArgumentsAnalysisLPM();
  ~AggregateArgumentsAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return AggregateArgumentsAnalysis::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return AggregateArgumentsAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                            getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class AggregateArgumentsAnalysisNPM : public llvm::PassInfoMixin<AggregateArgumentsAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-agg-arg-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveAggregateArguments {
public:
  ResolveAggregateArguments() {}

  static llvm::StringRef getPassName() { return "ResolveAggregateArguments"; }

  bool runOnFunction(llvm::Function &F, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

private:
  void storeArgument(const llvm::Argument *, llvm::AllocaInst *base, IGCLLVM::IRBuilder<> &irBuilder);

  void getImplicitArg(unsigned int explicitArgNo, unsigned int &startArgNo, unsigned int &endArgNo);

protected:
  llvm::Function *m_pFunction = nullptr;

  ImplicitArgs m_implicitArgs;
};

// Legacy Pass Manager wrapper.
class ResolveAggregateArgumentsLPM : public llvm::FunctionPass {
public:
  static char ID;

  ResolveAggregateArgumentsLPM();
  ~ResolveAggregateArgumentsLPM() {}

  virtual llvm::StringRef getPassName() const override { return ResolveAggregateArguments::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }

private:
  ResolveAggregateArguments m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions (the seeded MetaDataUtilsAnalysis is module-level; IGC passes never use
// skipFunction). name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class ResolveAggregateArgumentsNPM : public llvm::PassInfoMixin<ResolveAggregateArgumentsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-agg-arg"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
