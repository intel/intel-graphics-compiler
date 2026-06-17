/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvm/ADT/StringSwitch.h"
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/IRBuilder.h>

namespace IGC {
class CodeGenContext;
}

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BfloatFuncsResolution : public llvm::InstVisitor<BfloatFuncsResolution> {
public:
  BfloatFuncsResolution() {}
  ~BfloatFuncsResolution() {}

  static llvm::StringRef getPassName() { return "BfloatFuncsResolution"; }

  bool runOnFunction(llvm::Function &F, IGC::CodeGenContext *pCtx);

  void visitCallInst(llvm::CallInst &CI);

#if LLVM_VERSION_MAJOR >= 14
  // Below functions use bfloat type. We Leave the visitCallInst function
  // to detect if the builtins were used on unsupported LLVM version.
private:
  void handleCompare(llvm::CallInst &CI, llvm::CmpInst::Predicate Pred);
  void handleSelect(llvm::CallInst &CI);
  void handleMinMax(llvm::CallInst &CI, llvm::CmpInst::Predicate Pred);
  void handleArithmetic(llvm::CallInst &CI, unsigned Opcode, bool isMadInstruction = false);
  void handleMath(llvm::CallInst &CI, llvm::Intrinsic::ID Operation, bool needsGenISAIntrinsic = false);

  // Helpers.
  llvm::Value *bitcastToBfloat(llvm::Value *V);
  llvm::Type *getTypeBasedOnType(llvm::Type *OrgType, llvm::Type *DesiredTypeScalar);
#endif
  bool m_changed = false;
  std::vector<llvm::Instruction *> m_instructionsToRemove;
  llvm::IRBuilder<> *m_builder = nullptr;
  IGC::CodeGenContext *m_ctx = nullptr;
};

// Legacy Pass Manager wrapper.
class BfloatFuncsResolutionLPM : public llvm::FunctionPass {
public:
  static char ID;

  BfloatFuncsResolutionLPM();
  ~BfloatFuncsResolutionLPM() {}

  virtual llvm::StringRef getPassName() const override { return BfloatFuncsResolution::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnFunction(llvm::Function &F) override {
    return m_impl.runOnFunction(F, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  BfloatFuncsResolution m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined
// functions (the seeded CodeGenContextAnalysis is module-level; IGC passes never use
// skipFunction). name() returns the legacy pass argument so PrintBefore/PrintAfter
// matches under the new pass manager.
class BfloatFuncsResolutionNPM : public llvm::PassInfoMixin<BfloatFuncsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-bfloat-funcs-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
