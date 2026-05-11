/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cstdint>

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "AdaptorOCL/Utils/CacheControlsHelper.h"

#include <sstream>

namespace IGC {
class Subgroup2DBlockIoResolution final : public llvm::ModulePass,
                                          public llvm::InstVisitor<Subgroup2DBlockIoResolution> {
public:
  static char ID;

  Subgroup2DBlockIoResolution();
  ~Subgroup2DBlockIoResolution() = default;

  virtual llvm::StringRef getPassName() const override { return "Subgroup2DBlockIoResolution"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }
  virtual bool runOnModule(llvm::Module &M) override;
  void visitCallInst(llvm::CallInst &CI);

private:
  struct BlockDescription {
    uint32_t ElemBitwidth = 0;
    // in elements
    uint32_t Width = 0;
    uint32_t Height = 0;
    uint32_t NumBlocksV = 0;
  };

  struct MemoryDesc {
    llvm::Value *GlobalMem = nullptr;
    llvm::Value *Width = nullptr;
    llvm::Value *Height = nullptr;
    llvm::Value *Pitch = nullptr;
    llvm::Value *Coord = nullptr;
    llvm::Value *PrivateMem = nullptr;
  };

  enum class Op { Read, ReadTransform, ReadTranspose, Prefetch, Write, Invalid };

  struct Constraints {
    uint32_t ElemBitwidth, MinHeight, MaxHeight, MinWidth, MaxWidth, MinBlocksV, MaxBlocksV, MinElements, MaxElements;
  };

  void visit2DBlockCallInst(llvm::CallInst &CI, const llvm::SmallVector<llvm::StringRef, 3> &Matches, bool IsSPIRV);

  static Op parseOperation(llvm::StringRef OpStr);
  static MemoryDesc extractMemoryDescription(llvm::CallInst &CI, Op OpType, bool IsSPIRV);
  BlockDescription extractBlockDescription(llvm::CallInst &CI);
  static BlockDescription parseBlockDescription(llvm::StringRef Description);
  bool isConstantInt(llvm::Value *Val, llvm::StringRef ValName);
  void emitAdditionalConstraintsError(llvm::CallInst &CI, const BlockDescription &Desc, bool WillReadMoreThanLoaded,
                                      bool Other);
  static bool validateHWConstraints(const BlockDescription &Desc, const llvm::ArrayRef<Constraints> ConstraintsArray);
  bool validateBlockDescription(llvm::CallInst &CI, const Op OpType, const BlockDescription &Desc);
  bool validateBlockDescriptionForRead(llvm::CallInst &CI, const BlockDescription &Desc);
  bool validateBlockDescriptionForReadTransform(llvm::CallInst &CI, const BlockDescription &Desc);
  bool validateBlockDescriptionForReadTranspose(llvm::CallInst &CI, const BlockDescription &Desc);
  bool validateBlockDescriptionForPrefetch(llvm::CallInst &CI, const BlockDescription &Desc);
  bool validateBlockDescriptionForWrite(llvm::CallInst &CI, const BlockDescription &Desc);
  template <typename CCT>
  CacheControlFromMDNodes resolveCacheControlDecorations(llvm::CallInst &CI, llvm::Value *PointerValue, Op OpType);
  llvm::SmallVector<llvm::Value *, 14> getArgs(llvm::CallInst &CI, const MemoryDesc &MemDesc,
                                               const BlockDescription &BlockDesc, Op OpType,
                                               CacheControlFromMDNodes CacheControls);
  llvm::Type *getOverloadedType(llvm::CallInst &CI, Op OpType, const BlockDescription &BlockDesc);
  llvm::Type *getOverloadedTypeCommon(llvm::CallInst &CI, const BlockDescription &BlockDesc);
  llvm::Type *getOverloadedTypeForTransform(llvm::CallInst &CI, const BlockDescription &BlockDesc);
  static llvm::GenISAIntrinsic::ID getIntrinsic(Op OpType);
  void mergeBlocks(Op OpType, BlockDescription &BlockDesc);

  // Variadic template function to emit warnings with formatted message
  template <typename... Args> void emitWarning(const llvm::Value &Context, Args &&...Parts) {
    std::ostringstream Ss;
    (Ss << ... << std::forward<Args>(Parts));
    m_Ctx->EmitWarning(Ss.str().c_str(), &Context);
  }

  template <typename... Args> void emitError(const llvm::Value &Context, Args &&...Parts) {
    std::ostringstream Ss;
    (Ss << ... << std::forward<Args>(Parts));
    m_Ctx->EmitError(Ss.str().c_str(), &Context);
  }

  bool m_Changed = false;
  uint32_t m_SimdSize = 0;
  uint32_t m_GRFSize = 0;
  IGC::CodeGenContext *m_Ctx = nullptr;
  llvm::DenseSet<llvm::Function *> m_BuiltinsToRemove;
};
} // namespace IGC
