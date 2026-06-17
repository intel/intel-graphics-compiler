/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  PrivateMemoryUsageAnalysis pass used for analyzing if functions use private memory.
///         This is done by analyzing the alloca instructions.

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PrivateMemoryUsageAnalysis : public llvm::InstVisitor<PrivateMemoryUsageAnalysis> {
public:
  /// @brief  Constructor
  PrivateMemoryUsageAnalysis() {}

  /// @brief  Destructor
  ~PrivateMemoryUsageAnalysis() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "PrivateMemoryUsageAnalysis"; }

  /// @brief  Main entry point.
  ///         Runs on all functions defined in the given module, finds all alloca instructions,
  ///         analyzes them and checks if the functions use private memory.
  ///         If so, private base implicit argument is added to the implicit arguments of the function
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);

  /// @brief  Alloca instructions visitor.
  ///         Analyzes if there are private memory allocation.
  /// @param  AI The alloca instruction.
  void visitAllocaInst(llvm::AllocaInst &AI);

  /// @brief  BinaryOperator instructions visitor.
  ///         Analyzes if there are private memory allocation.
  /// @param  I The binary op
  void visitBinaryOperator(llvm::BinaryOperator &I);

  /// @brief  CallInst instructions visitor.
  ///         Analyzes if there are private memory allocation.
  /// @param  CI The binary op
  void visitCallInst(llvm::CallInst &CI);

  /// @brief  LoadInst instructions visitor.
  ///         Analyzes if there are struct types loaded.
  /// @param  LI The load instruction.
  void visitLoadInst(llvm::LoadInst &LI);

  /// @brief  StoreInst instructions visitor.
  ///         Analyzes if there are struct types stored.
  /// @param  SI The store instruction.
  void visitStoreInst(llvm::StoreInst &SI);

  /// @brief  GetElementPtrInst instructions visitor.
  ///         Analyzes if there are struct types accessed.
  /// @param  GEP The GEP instruction.
  void visitGetElementPtrInst(llvm::GetElementPtrInst &GEP);

private:
  /// @brief  Function entry point.
  ///         Finds all alloca instructions in this function, analyzes them and adds
  ///         private base implicit argument if needed.
  /// @param  F The destination function.
  bool runOnFunction(llvm::Function &F);

  /// @brief  Type visitor.
  ///         Checks if the type is a non-opaque struct.
  /// @param  Ty The type to inspect.
  inline void visitType(llvm::Type *Ty);

  /// @brief  A flag signaling if the current function uses private memory
  bool m_hasPrivateMem = false;

  /// @brief A flag signaling if the platform has partial fp64 emulation
  bool m_hasDPDivSqrtEmu = false;

  /// @brief  MetaData utils used to generate LLVM metadata
  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;

  /// @brief  Compilation context, cached at run() entry.
  IGC::CodeGenContext *m_pCtx = nullptr;
};

// Legacy Pass Manager wrapper.
class PrivateMemoryUsageAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  PrivateMemoryUsageAnalysisLPM();
  ~PrivateMemoryUsageAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return PrivateMemoryUsageAnalysis::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return PrivateMemoryUsageAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                            getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class PrivateMemoryUsageAnalysisNPM : public llvm::PassInfoMixin<PrivateMemoryUsageAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-private-mem-usage-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
