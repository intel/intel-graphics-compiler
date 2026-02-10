/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

namespace IGC {
class MinimumValidAddressChecking : public llvm::ModulePass, public llvm::InstVisitor<MinimumValidAddressChecking> {
public:
  static char ID;

  MinimumValidAddressChecking(uint64_t minimumValidAddress = 0);
  ~MinimumValidAddressChecking() = default;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &analysisUsage) const override {
    analysisUsage.addRequired<MetaDataUtilsWrapper>();
    analysisUsage.addRequired<CodeGenContextWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return "MinimumValidAddressChecking"; }

  virtual bool runOnModule(llvm::Module &M) override;

  void visitLoadInst(llvm::LoadInst &load);
  void visitStoreInst(llvm::StoreInst &store);

private:
  uint64_t minimumValidAddress;

  llvm::SmallVector<llvm::Instruction *, 4> loadsAndStoresToCheck;
  llvm::DenseMap<llvm::StringRef, llvm::GlobalVariable *> stringsCache;
  llvm::DICompileUnit *compileUnit;

  void handleLoadStore(llvm::Instruction *instruction);
  llvm::Value *createLoadStoreReplacement(llvm::Instruction *instruction, llvm::Instruction *insertBefore);
  void createAssertCall(llvm::Value *address, llvm::Instruction *instruction, llvm::Instruction *insertBefore);
  llvm::SmallVector<llvm::Value *, 4> createAssertArgs(llvm::Value *address, llvm::Instruction *instruction,
                                                       llvm::Instruction *insertBefore);
  llvm::GlobalVariable *getOrCreateGlobalConstantString(llvm::Module *M, llvm::StringRef format);
};
} // namespace IGC
