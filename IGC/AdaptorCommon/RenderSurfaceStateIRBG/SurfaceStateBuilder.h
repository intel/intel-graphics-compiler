/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "RenderSurfaceStateBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

/// Simple context adapter for standalone SurfaceStateBuilder usage.
/// Provides the interface expected by the CRTP mixin (getLLVMContext/getModule).
class SurfaceStateBuilderContext {
  llvm::Module *M;

public:
  explicit SurfaceStateBuilderContext(llvm::Module *M) : M(M) {}

  llvm::LLVMContext *getLLVMContext() const { return &M->getContext(); }
  llvm::Module *getModule() const { return M; }
};

/// Lightweight builder class that provides access to generated RENDER_SURFACE_STATE accessors.
/// Only include this header in files that need surface state access.
///
/// For use within RTBuilder or other builders that have an IGC::CodeGenContext,
/// prefer inheriting from RenderSurfaceStateBuilder<Derived> directly.
class SurfaceStateBuilder : public llvm::IRBuilder<>, public llvm::RenderSurfaceStateBuilder<SurfaceStateBuilder> {
  SurfaceStateBuilderContext CtxAdapter;
  unsigned SurfaceStateSize;

public:
  SurfaceStateBuilder(llvm::Module *M, unsigned SurfaceStateSize)
      : llvm::IRBuilder<>(M->getContext()), CtxAdapter(M), SurfaceStateSize(SurfaceStateSize) {}

  SurfaceStateBuilder(llvm::BasicBlock *BB, unsigned SurfaceStateSize)
      : llvm::IRBuilder<>(BB), CtxAdapter(BB->getModule()), SurfaceStateSize(SurfaceStateSize) {}

  SurfaceStateBuilder(llvm::Instruction *IP, unsigned SurfaceStateSize)
      : llvm::IRBuilder<>(IP), CtxAdapter(IP->getModule()), SurfaceStateSize(SurfaceStateSize) {}

  const SurfaceStateBuilderContext &getCtx() const { return CtxAdapter; }
  unsigned getSurfaceStateSize() const { return SurfaceStateSize; }
};

} // namespace IGC
