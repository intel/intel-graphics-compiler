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
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/IRBuilder.h>

namespace IGC {
class CodeGenContext;
}

namespace IGC {
class BfloatFuncsResolution : public llvm::FunctionPass, public llvm::InstVisitor<BfloatFuncsResolution> {
public:
  static char ID;

  BfloatFuncsResolution();

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnFunction(llvm::Function &F) override;

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

} // namespace IGC
