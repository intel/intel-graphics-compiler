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

class CatchAllLineNumber : public llvm::FunctionPass {
public:
  CatchAllLineNumber();
  virtual ~CatchAllLineNumber();

  CatchAllLineNumber(const CatchAllLineNumber &) = delete;
  CatchAllLineNumber &operator=(const CatchAllLineNumber &) = delete;

  static char ID;

  llvm::StringRef getPassName() const override { return "CatchAllLineNumber"; }

private:
  virtual bool runOnFunction(llvm::Function &F) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.setPreservesAll(); }
};
}; // namespace IGC
