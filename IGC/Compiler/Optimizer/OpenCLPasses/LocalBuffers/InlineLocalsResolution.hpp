/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/PassManager.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Analysis/CallGraph.h>
#include "common/LLVMWarningsPop.hpp"

#include <map>
#include <set>
#include <unordered_set>

namespace IGC {
typedef llvm::SetVector<llvm::GlobalVariable *> GlobalVariableSet;

/// @brief  This pass resolves references to inline local address space variables
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class InlineLocalsResolution {
public:
  /// @brief  Constructor
  InlineLocalsResolution() {}

  /// @brief  Destructor
  ~InlineLocalsResolution() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "InlineLocalsResolutionPass"; }

  /// @brief  Main entry point.
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD, IGC::CodeGenContext *pCtx,
           llvm::CallGraph &CG);

protected:
  void filterGlobals(llvm::Module &);
  bool unusedGlobal(llvm::Value *V, std::unordered_set<llvm::Value *> &unusedNodes);
  void collectInfoOnSharedLocalMem(llvm::Module &);
  void computeOffsetList(llvm::Module &, llvm::MapVector<llvm::Function *, unsigned int> &);
  void traverseCGN(const llvm::CallGraphNode &);

private:
  llvm::MapVector<llvm::Function *, GlobalVariableSet> m_FuncToVarsMap;
  llvm::MapVector<llvm::Function *, unsigned int> m_FuncToMemPoolSizeMap;
  std::set<llvm::Function *> m_chkSet;
  llvm::GlobalVariable *m_pGV = nullptr;

  // Context cached at run() entry.
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  ModuleMetaData *m_modMD = nullptr;
  IGC::CodeGenContext *m_pCtx = nullptr;
  llvm::CallGraph *m_CG = nullptr;
};

// Legacy Pass Manager wrapper.
class InlineLocalsResolutionLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  InlineLocalsResolutionLPM();
  ~InlineLocalsResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return InlineLocalsResolution::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                      getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph());
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
  }

private:
  InlineLocalsResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. The call graph comes from LLVM's standard CallGraphAnalysis
// (registered by PassBuilder::registerModuleAnalyses in IGCNewPassManager). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class InlineLocalsResolutionNPM : public llvm::PassInfoMixin<InlineLocalsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-resolve-inline-locals"; }
  static bool isRequired() { return true; }

private:
  InlineLocalsResolution m_impl;
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
