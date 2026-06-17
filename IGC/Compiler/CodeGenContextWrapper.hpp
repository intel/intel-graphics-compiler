/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/Types.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class CodeGenContext;
class CBTILayout;
class CPlatform;
class CDriverInfo;
// This pass provides access to  CodeGenContext.
//
// To use this from within another pass:
//  1. Add CodeGenContextWrapper to the pass manager.
//  2. Use getAnalysisIfAvailable on CodeGenContextWrapper:
//       CodeGenContextWrapper* pCtxWrapper = getAnalysis<CodeGenContextWrapper>();
//  3. Get the CodeGenContext:
//      CodeGenContext* ctx = pCtxWrapper->getCodeGenContext();

class CodeGenContextWrapper : public llvm::ImmutablePass {
public:
  static char ID;

  // Constructs a wrapper to the given CodeGenContext instance.
  CodeGenContextWrapper(CodeGenContext *pCtx);
  CodeGenContextWrapper();

  // return the Context
  CodeGenContext *getCodeGenContext();

  virtual llvm::StringRef getPassName() const override { return "CodeGen Context Wrapper"; }

private:
  CodeGenContext *m_ctx;
};

#if LLVM_VERSION_MAJOR >= 16
// Result of CodeGenContextAnalysis. The analysis result must be a class/struct
// type (the analysis manager inherits from it to detect an invalidate() method),
// so the CodeGenContext* is wrapped rather than returned bare.
struct CodeGenContextResult {
  CodeGenContext *Ctx = nullptr;
};

// New Pass Manager analysis exposing the CodeGenContext to ported passes.
//
// The result wraps the same CodeGenContext* held by the legacy
// CodeGenContextWrapper ImmutablePass. It is seeded externally (the analysis does
// not compute anything from the module); register it with a ModuleAnalysisManager
// via:
//   MAM.registerPass([Ctx] { return CodeGenContextAnalysis(Ctx); });
// and retrieve it in an NPM pass via:
//   CodeGenContext *ctx = AM.getResult<CodeGenContextAnalysis>(M).Ctx;
class CodeGenContextAnalysis : public llvm::AnalysisInfoMixin<CodeGenContextAnalysis> {
  friend llvm::AnalysisInfoMixin<CodeGenContextAnalysis>;
  static llvm::AnalysisKey Key;

  CodeGenContext *m_ctx = nullptr;

public:
  using Result = CodeGenContextResult;

  CodeGenContextAnalysis() = default;
  explicit CodeGenContextAnalysis(CodeGenContext *ctx) : m_ctx(ctx) {}

  Result run(llvm::Module &, llvm::ModuleAnalysisManager &) { return Result{m_ctx}; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
