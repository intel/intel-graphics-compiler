/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/MapVector.h>
#include "common/LLVMWarningsPop.hpp"
#include <common/Types.hpp>
#include <common/allocator.h>
#include <common/Stats.hpp>

namespace llvm {
class BasicBlock;
}

namespace IGC {
class CShader;
class CodeGenContext;

/// This class contains the information for the different SIMD version
/// of a kernel. Each kernel in the module is associated to one CShaderProgram
class CShaderProgram {
public:
  typedef llvm::MapVector<llvm::Function *, CShaderProgram *> KernelShaderMap;
  CShaderProgram(CodeGenContext *ctx, llvm::Function *kernel);
  ~CShaderProgram();
  CShaderProgram(const CShaderProgram &) = delete;
  CShaderProgram &operator=(const CShaderProgram &) = delete;
  CShader *GetOrCreateShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
  CShader *GetShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
  CShader *GetShaderIfAny(ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
  void DeleteShader(SIMDMode simd, ShaderDispatchMode mode = ShaderDispatchMode::NOT_APPLICABLE);
  CodeGenContext *GetContext() { return m_context; }

  llvm::Function *getLLVMFunction() const { return m_kernel; }
  ShaderStats *m_shaderStats = nullptr;

  // invoked to clear Func ptr when the current module is deleted (so is func within it).
  void clearBeforeRetry();

  bool hasShaderOutput(CShader *shader);

  void freeShaderOutput(CShader *shader);

  void ClearShaderPtr(SIMDMode simd);

  // EnableSampleTailDeAlias peak-aware experiment: cache the function's highest
  // register-pressure basic block (the block ranking is SIMD-independent), so
  // EmitPass computes it once per function instead of once per SIMD EmitPass
  // instance. Keyed by Function* to stay correct for multi-function groups.
  // Presence in the map means "computed" (value may be null for a trivial fn).
  bool hasSampleTailPeakBB(llvm::Function *F) const { return m_sampleTailPeakBB.count(F) != 0; }
  llvm::BasicBlock *getSampleTailPeakBB(llvm::Function *F) const {
    auto It = m_sampleTailPeakBB.find(F);
    return It != m_sampleTailPeakBB.end() ? It->second : nullptr;
  }
  void setSampleTailPeakBB(llvm::Function *F, llvm::BasicBlock *BB) { m_sampleTailPeakBB[F] = BB; }

protected:
  CShader *&GetShaderPtr(SIMDMode simd, ShaderDispatchMode mode);
  CShader *CreateNewShader(SIMDMode simd);

  CodeGenContext *m_context = nullptr;
  llvm::Function *m_kernel = nullptr;
  std::array<CShader *, 10> m_SIMDshaders;
  // EnableSampleTailDeAlias peak-aware experiment: see hasSampleTailPeakBB().
  llvm::DenseMap<llvm::Function *, llvm::BasicBlock *> m_sampleTailPeakBB;

public:
  typedef std::unique_ptr<CShaderProgram> UPtr;
};
} // namespace IGC