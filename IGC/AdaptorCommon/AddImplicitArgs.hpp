/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include <set>
#include <map>

namespace IGC {
class CodeGenContext;
}

namespace IGC {
// struct
typedef std::map<unsigned int, llvm::Argument *> InfoToImpArgMap;
typedef std::map<llvm::Function *, InfoToImpArgMap> FuncInfoToImpArgMap;

typedef std::map<llvm::Argument *, unsigned int> ImpArgToExpNum;
typedef std::map<llvm::Function *, ImpArgToExpNum> FuncImpToExpNumMap;

/// @brief  AddImplicitArgs pass used for changing the function signatures and calls to represent
///         the implicit arguments needed by each function and passed from the OpenCL runtime
///         This pass depdends on the WIFuncAnalysis pass runing before it
/// @Author Marina Yatsina
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class AddImplicitArgs {
public:
  /// @brief  Constructor
  AddImplicitArgs() {}

  /// @brief  Destructor
  ~AddImplicitArgs() {}

  /// @brief  Provides name of pass
  static llvm::StringRef getPassName() { return "AddImplicitArgs"; }

  /// @brief  Main entry point.
  ///         Goes over all functions and changes their signature to contain the implicit arguments
  ///         needed by each function, goes over all function calls and adds the implicit arguments
  ///         to the function calls
  /// @param  M The destination module.
  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx);

  /// @brief  Check if a function has a stackcall in its call path to
  ///         decide whether implicit args should be added
  /// @param  pFunc           Source function
  static bool hasStackCallInCG(const llvm::Function *pFunc);

private:
  /// @brief  Create the type of the new function,
  ///         including all explicit and needed impliict parameters
  /// @param  pFunc           The old function
  /// @param  pImplicitArgs   The implicit arguments needed by this function
  /// @returns    The new function type
  static llvm::FunctionType *getNewFuncType(const llvm::Function *pFunc, const ImplicitArgs &implicitArgs);

  /// @brief  Transfers uses of old arguments to new arguments, sets names of all arguments
  /// @param  pFunc           The old function
  /// @param  pNewFunc        The new function
  /// @param  pImplicitArgs   The implicit arguments needed by this function
  void updateNewFuncArgs(llvm::Function *pFunc, llvm::Function *pNewFunc, const ImplicitArgs &implicitArgs);

  /// @brief  Replace old CallInst with new CallInst
  void replaceAllUsesWithNewOCLBuiltinFunction(llvm::Function *old_func, llvm::Function *new_func);

  static llvm::Value *coerce(llvm::Value *arg, llvm::Type *type, llvm::Instruction *insertBefore);

  /// @brief  Metadata API object.
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  /// @brief  Compilation context, cached at run() entry.
  IGC::CodeGenContext *m_ctx = nullptr;

  FuncInfoToImpArgMap m_FuncInfoToImpArgMap;
  FuncImpToExpNumMap m_FuncImpToExpNumMap;
};

// Legacy Pass Manager wrapper.
class AddImplicitArgsLPM : public llvm::ModulePass {
public:
  // Pass identification, replacement for typeid
  static char ID;

  AddImplicitArgsLPM();
  ~AddImplicitArgsLPM() {}

  virtual llvm::StringRef getPassName() const override { return AddImplicitArgs::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  AddImplicitArgs m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class AddImplicitArgsNPM : public llvm::PassInfoMixin<AddImplicitArgsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-add-implicit-args"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC

// Builtin CallGraph Analysis Class
namespace IGC {

struct ImplicitArgumentDetail {
  ImplicitArg::ArgMap ArgsMaps;
  std::set<unsigned int> StructArgSet;
};

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BuiltinCallGraphAnalysis {
public:
  BuiltinCallGraphAnalysis() {}
  ~BuiltinCallGraphAnalysis() {}

  static llvm::StringRef getPassName() { return "BuiltinCallGraphAnalysis"; }

  bool run(llvm::Module &M, IGC::IGCMD::MetaDataUtils *pMdUtils, IGC::CodeGenContext *pCtx, llvm::CallGraph &CG);

  void traverseCallGraphSCC(const std::vector<llvm::CallGraphNode *> &SCCNodes);
  void combineTwoArgDetail(ImplicitArgumentDetail &, const ImplicitArgumentDetail &, llvm::Value *) const;
  void writeBackAllIntoMetaData(const ImplicitArgumentDetail &, llvm::Function *);

  bool pruneCallGraphForStackCalls(llvm::CallGraph &CG);

private:
  IGC::IGCMD::MetaDataUtils *m_pMdUtils = nullptr;
  IGC::CodeGenContext *m_ctx = nullptr;
  llvm::SmallDenseMap<llvm::Function *, ImplicitArgumentDetail *> argMap;
  llvm::SmallVector<std::unique_ptr<ImplicitArgumentDetail>, 4> argDetails;
};

// Legacy Pass Manager wrapper.
class BuiltinCallGraphAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  BuiltinCallGraphAnalysisLPM();
  ~BuiltinCallGraphAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return BuiltinCallGraphAnalysis::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<llvm::CallGraphWrapperPass>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return m_impl.run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext(),
                      getAnalysis<llvm::CallGraphWrapperPass>().getCallGraph());
  }

private:
  BuiltinCallGraphAnalysis m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. The call graph comes from LLVM's standard CallGraphAnalysis
// (registered by PassBuilder::registerModuleAnalyses in IGCNewPassManager). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class BuiltinCallGraphAnalysisNPM : public llvm::PassInfoMixin<BuiltinCallGraphAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-callgraphscc-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
