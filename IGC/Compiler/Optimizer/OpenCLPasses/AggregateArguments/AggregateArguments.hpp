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
#include "common/LLVMWarningsPop.hpp"
#include <llvmWrapper/IR/IRBuilder.h>

namespace IGC {
class AggregateArgumentsAnalysis : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  AggregateArgumentsAnalysis();

  virtual llvm::StringRef getPassName() const override { return "AggregateArgumentsAnalysis"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override;

private:
  void addImplictArgs(llvm::Type *type, uint64_t baseAllocaOffset);

private:
  const llvm::DataLayout *m_pDL{};
  ImplicitArg::StructArgList m_argList;
  IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
};

class ResolveAggregateArguments : public llvm::FunctionPass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  ResolveAggregateArguments();

  virtual llvm::StringRef getPassName() const override { return "ResolveAggregateArguments"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  void storeArgument(const llvm::Argument *, llvm::AllocaInst *base, IGCLLVM::IRBuilder<> &irBuilder);

  void getImplicitArg(unsigned int explicitArgNo, unsigned int &startArgNo, unsigned int &endArgNo);

protected:
  llvm::Function *m_pFunction = nullptr;

  ImplicitArgs m_implicitArgs;
};

} // namespace IGC
