/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
/// SPV_EXT_long_vector allows vector widths BiF has no builtins for. A call to
/// a SPIR-V friendly builtin that BiF doesn't define becomes one call of the
/// scalar BiF variant per element, or a dedicated lowering where the vector
/// form isn't elementwise (dot, any, shuffle, vloadn, ...).
///
/// Shared implementation. Holds the logic and is used by both the legacy and
/// the new-pass-manager wrappers below; it is not itself an llvm::Pass.
class LongVectorBuiltinScalarization final {
public:
  static llvm::StringRef getPassName() { return "LongVectorBuiltinScalarization"; }

  bool run(llvm::Module &M, CodeGenContext *Ctx);
};

// Legacy Pass Manager wrapper.
class LongVectorBuiltinScalarizationLPM final : public llvm::ModulePass {
public:
  static char ID;

  LongVectorBuiltinScalarizationLPM();

  llvm::StringRef getPassName() const override { return LongVectorBuiltinScalarization::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  bool runOnModule(llvm::Module &M) override {
    return LongVectorBuiltinScalarization().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class LongVectorBuiltinScalarizationNPM : public llvm::PassInfoMixin<LongVectorBuiltinScalarizationNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-long-vector-builtin-scalarization"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
