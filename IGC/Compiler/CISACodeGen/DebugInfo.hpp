/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "llvm/Config/llvm-config.h"
#include "common/LLVMWarningsPush.hpp"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "DebugInfo/VISAModule.hpp"
#include "DebugInfo/VISAIDebugEmitter.hpp"
#include "ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"
#include "llvm/IR/DIBuilder.h"
#include "llvm/IR/PassManager.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;
using namespace std;

namespace IGC {
class CVariable;
class VISADebugInfo;

class DebugInfoPass : public llvm::ModulePass {
public:
  DebugInfoPass();
  DebugInfoPass(CShaderProgram::KernelShaderMap &);
  virtual llvm::StringRef getPassName() const override { return "DebugInfoPass"; }
  virtual ~DebugInfoPass();

  DebugInfoPass(const DebugInfoPass &) = delete;
  DebugInfoPass &operator=(const DebugInfoPass &) = delete;

  static char ID;

private:
  CShaderProgram::KernelShaderMap &kernels;
  CShader *m_currShader = nullptr;
  IDebugEmitter *m_pDebugEmitter = nullptr;

  virtual bool runOnModule(llvm::Module &M) override;
  virtual bool doInitialization(llvm::Module &M) override;
  virtual bool doFinalization(llvm::Module &M) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.setPreservesAll();
  }

  void EmitDebugInfo(bool, const IGC::VISADebugInfo &VDI);
};

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class CatchAllLineNumber {
public:
  CatchAllLineNumber() {}
  ~CatchAllLineNumber() {}

  static llvm::StringRef getPassName() { return "CatchAllLineNumber"; }

  bool run(llvm::Function &F);
};

// Legacy Pass Manager wrapper.
class CatchAllLineNumberLPM : public llvm::FunctionPass {
public:
  CatchAllLineNumberLPM();
  virtual ~CatchAllLineNumberLPM();

  CatchAllLineNumberLPM(const CatchAllLineNumberLPM &) = delete;
  CatchAllLineNumberLPM &operator=(const CatchAllLineNumberLPM &) = delete;

  static char ID;

  llvm::StringRef getPassName() const override { return CatchAllLineNumber::getPassName(); }

private:
  virtual bool runOnFunction(llvm::Function &F) override { return m_impl.run(F); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesAll(); }

  CatchAllLineNumber m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. No analysis dependencies, so a plain function pass. name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class CatchAllLineNumberNPM : public llvm::PassInfoMixin<CatchAllLineNumberNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-catch-all-linenum"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
}; // namespace IGC
