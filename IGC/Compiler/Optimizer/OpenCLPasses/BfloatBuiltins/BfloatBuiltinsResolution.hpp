/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/IRBuilder.h>
#include <llvmWrapper/IR/Instructions.h>

namespace IGC {
class CodeGenContext;
}

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BfloatBuiltinsResolution : public llvm::InstVisitor<BfloatBuiltinsResolution> {
public:
  BfloatBuiltinsResolution() {}

  static llvm::StringRef getPassName() { return "BfloatBuiltinsResolution"; }

  bool run(llvm::Function &F);
  void visitCallInst(llvm::CallInst &CI);

  struct CallInstNamePair {
    llvm::CallInst *CI;
    std::string NewName;
  };
  void resolveCallInstPair(const CallInstNamePair &pair);

  std::vector<CallInstNamePair> CallInstPairs;
};

// Legacy Pass Manager wrapper.
class BfloatBuiltinsResolutionLPM : public llvm::FunctionPass {
public:
  static char ID;
  BfloatBuiltinsResolutionLPM();

  llvm::StringRef getPassName() const override { return BfloatBuiltinsResolution::getPassName(); }

  bool runOnFunction(llvm::Function &F) override { return BfloatBuiltinsResolution().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class BfloatBuiltinsResolutionNPM : public llvm::PassInfoMixin<BfloatBuiltinsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-bfloat-builtins-resolution"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
