/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

namespace IGC {
// This pass performs the following optimization:
//
//  It replaces the work group reduce instruction which is only used by
//  the work item with local or global id = 0 an optimised reduce.
//  An optimized reduce uses non-zero work items less than a regular one.
//  Applies to workspace dimension equals 3 only.

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ReduceOptPass {
public:
  ReduceOptPass() {}

  static llvm::StringRef getPassName() { return "Reduce Optimisation Pass"; }

  bool run(llvm::Function &F);

private:
  // This function recursively checks if the result of the reduce
  // function is only used by the work item with the local_linear_id == 0
  // or global_linear_id == 0.
  bool checkUsers(llvm::Value *MainVal, llvm::Value *Val, llvm::BasicBlock *BB);

  // Helper functions for checking the condition of using the reduce result
  bool checkSelect(llvm::Instruction *Sel);
  bool checkBranch(llvm::BasicBlock *Bb);
  bool checkCmp(llvm::Value *Val);
  bool checkGlobalId(llvm::Value *Val);
  bool checkLocalId(llvm::Value *Val);
  bool checkBuiltInName(llvm::Value *I, const std::string &Name);

  bool createReduceWI0(llvm::Instruction *ReduceInstr);

  bool Changed = false;
  llvm::Module *M = nullptr;
};

// Legacy Pass Manager wrapper.
class ReduceOptPassLPM : public llvm::FunctionPass {
public:
  static char ID;

  ReduceOptPassLPM();

  llvm::StringRef getPassName() const override { return ReduceOptPass::getPassName(); }

  bool runOnFunction(llvm::Function &F) override { return ReduceOptPass().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ReduceOptPassNPM : public llvm::PassInfoMixin<ReduceOptPassNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "opt-reduce-pass"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC