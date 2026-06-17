/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class DeviceEnqueueFuncsAnalysis : public llvm::InstVisitor<DeviceEnqueueFuncsAnalysis> {
public:
  DeviceEnqueueFuncsAnalysis() {}
  ~DeviceEnqueueFuncsAnalysis() {}

  static llvm::StringRef getPassName() { return "DeviceEnqueueFuncsAnalysis"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  bool runOnFunction(llvm::Function &F);

  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_hasDeviceEnqueue = false;
  llvm::SmallVector<ImplicitArg::ArgType, ImplicitArg::NUM_IMPLICIT_ARGS> m_newImplicitArgs;
  ImplicitArg::ArgMap m_newNumberedImplicitArgs;
  IGCMD::MetaDataUtils *m_pMDUtils = nullptr;
  IGC::ModuleMetaData *m_modMD = nullptr;
};

// Legacy Pass Manager wrapper.
class DeviceEnqueueFuncsAnalysisLPM : public llvm::ModulePass {
public:
  static char ID;

  DeviceEnqueueFuncsAnalysisLPM();
  ~DeviceEnqueueFuncsAnalysisLPM() {}

  virtual llvm::StringRef getPassName() const override { return DeviceEnqueueFuncsAnalysis::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &M) override {
    return DeviceEnqueueFuncsAnalysis().run(M, getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils(),
                                            getAnalysis<MetaDataUtilsWrapper>().getModuleMetaData());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class DeviceEnqueueFuncsAnalysisNPM : public llvm::PassInfoMixin<DeviceEnqueueFuncsAnalysisNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-device-enqueue-func-analysis"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class DeviceEnqueueFuncsResolution : public llvm::InstVisitor<DeviceEnqueueFuncsResolution> {
public:
  DeviceEnqueueFuncsResolution() {}
  ~DeviceEnqueueFuncsResolution() {}

  static llvm::StringRef getPassName() { return "DeviceEnqueueFuncsResolution"; }

  bool runOnFunction(llvm::Function &F, IGCMD::MetaDataUtils *pMdUtils, IGC::ModuleMetaData *pModMD);

  void visitCallInst(llvm::CallInst &CI);

private:
  bool m_Changed = false;
  ImplicitArgs m_implicitArgs;
};

// Legacy Pass Manager wrapper.
class DeviceEnqueueFuncsResolutionLPM : public llvm::FunctionPass {
public:
  static char ID;

  DeviceEnqueueFuncsResolutionLPM();
  ~DeviceEnqueueFuncsResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return DeviceEnqueueFuncsResolution::getPassName(); }

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
  DeviceEnqueueFuncsResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions (the seeded MetaDataUtilsAnalysis is module-level; IGC passes never use
// skipFunction). name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class DeviceEnqueueFuncsResolutionNPM : public llvm::PassInfoMixin<DeviceEnqueueFuncsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-device-enqueue-func-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
