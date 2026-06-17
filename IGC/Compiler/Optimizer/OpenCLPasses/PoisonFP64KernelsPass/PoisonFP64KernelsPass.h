/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/Analysis/CallGraphSCCPass.h>
#include <llvm/ADT/SCCIterator.h>
#include "common/LLVMWarningsPop.hpp"

#include <vector>

namespace IGC {
// Shared implementation. Holds the logic + state and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class PoisonFP64Kernels {
public:
  static const char *attributeName;

  PoisonFP64Kernels() {}
  ~PoisonFP64Kernels() {}

  static llvm::StringRef getPassName() { return "Poison FP64 Kernels"; }

  /// @brief  Reset accumulated state (mirrors the legacy doInitialization).
  void reset();

  /// @brief  Process a single call-graph SCC (mirrors the legacy runOnSCC). SCC is the list of
  ///         CallGraphNodes in the strongly-connected component, visited bottom-up.
  bool runOnSCCNodes(const std::vector<llvm::CallGraphNode *> &SCC, CodeGenContext *ctx);

  /// @brief  Poison/erase the collected FP64 functions (mirrors the legacy doFinalization).
  bool finalize(CodeGenContext *ctx, IGCMD::MetaDataUtils *pMdUtils);

  /// @brief  Whole-module entry point used by the new-pass-manager wrapper: walks the call graph
  ///         SCCs bottom-up (the same order the CallGraphSCCPass framework uses) and finalizes.
  bool run(llvm::Module &M, llvm::CallGraph &CG, CodeGenContext *ctx, IGCMD::MetaDataUtils *pMdUtils);

  void markForRemoval(llvm::Function *F);

private:
  /* Use set as an index of found FP64 functions. Store the order in
   * which they were encountered in sequential container. Since this is a
   * CallGraphSCC pass the reverse of this order should give us the safe
   * removal order (from callers to callees) */
  llvm::SmallPtrSet<llvm::Function *, 8> fp64Functions;
  llvm::SmallVector<llvm::Function *, 8> fp64FunctionsOrder;
};

// Legacy Pass Manager wrapper. Kept as a CallGraphSCCPass to preserve the exact legacy
// scheduling/traversal; it delegates to the shared implementation.
class PoisonFP64KernelsLPM : public llvm::CallGraphSCCPass {
public:
  static char ID;

  PoisonFP64KernelsLPM();

  virtual llvm::StringRef getPassName() const override { return PoisonFP64Kernels::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool doInitialization(llvm::CallGraph &CG) override {
    m_impl.reset();
    return false;
  }

  virtual bool runOnSCC(llvm::CallGraphSCC &SCC) override {
    std::vector<llvm::CallGraphNode *> nodes(SCC.begin(), SCC.end());
    return m_impl.runOnSCCNodes(nodes, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

  virtual bool doFinalization(llvm::CallGraph &CG) override {
    return m_impl.finalize(getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                           getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils());
  }

private:
  PoisonFP64Kernels m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that walks the call-graph SCCs itself (the
// call graph comes from LLVM's standard CallGraphAnalysis, registered by registerModuleAnalyses).
// name() returns the legacy pass argument so PrintBefore/PrintAfter matches under the new PM.
class PoisonFP64KernelsNPM : public llvm::PassInfoMixin<PoisonFP64KernelsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-poison-fp64-kernels"; }
  static bool isRequired() { return true; }

private:
  PoisonFP64Kernels m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
