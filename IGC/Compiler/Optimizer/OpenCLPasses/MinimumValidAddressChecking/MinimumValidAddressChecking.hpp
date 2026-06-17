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
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

namespace IGC {
// Shared implementation. Holds the logic and the pass configuration (minimumValidAddress)
// and is used by both the legacy and the new-pass-manager wrappers below; it is not itself
// an llvm::Pass.
class MinimumValidAddressChecking : public llvm::InstVisitor<MinimumValidAddressChecking> {
public:
  MinimumValidAddressChecking(uint64_t minimumValidAddress = 0);
  ~MinimumValidAddressChecking() = default;

  static llvm::StringRef getPassName() { return "MinimumValidAddressChecking"; }

  bool run(llvm::Module &M);

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

// Legacy Pass Manager wrapper.
class MinimumValidAddressCheckingLPM : public llvm::ModulePass {
public:
  static char ID;

  MinimumValidAddressCheckingLPM(uint64_t minimumValidAddress = 0);
  ~MinimumValidAddressCheckingLPM() = default;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &analysisUsage) const override {
    analysisUsage.addRequired<MetaDataUtilsWrapper>();
    analysisUsage.addRequired<CodeGenContextWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return MinimumValidAddressChecking::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override { return m_impl.run(M); }

private:
  MinimumValidAddressChecking m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Carries the same configuration as the legacy pass via its
// constructor. name() returns the legacy pass argument so PrintBefore/PrintAfter matches
// under the new pass manager.
class MinimumValidAddressCheckingNPM : public llvm::PassInfoMixin<MinimumValidAddressCheckingNPM> {
public:
  MinimumValidAddressCheckingNPM(uint64_t minimumValidAddress = 0) : m_impl(minimumValidAddress) {}

  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-minimum-valid-address-checking"; }
  static bool isRequired() { return true; }

private:
  MinimumValidAddressChecking m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
