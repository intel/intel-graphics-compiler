/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"

#include <string>

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class HandleSpirvDecorationMetadata : public llvm::InstVisitor<HandleSpirvDecorationMetadata> {
public:
  HandleSpirvDecorationMetadata() {}
  ~HandleSpirvDecorationMetadata() {}

  static llvm::StringRef getPassName() { return "HandleSpirvDecorationMetadata"; }

  // Shared implementation used by both the legacy and the new-pass-manager wrappers.
  bool run(llvm::Module &module, CodeGenContext *pCtx, ModuleMetaData *modMD);

  void visitLoadInst(llvm::LoadInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitCallInst(llvm::CallInst &I);
  void visitBinaryOperator(llvm::BinaryOperator &I);
  void visit2DBlockReadCallInst(llvm::CallInst &I, llvm::StringRef unmangledName);
  void visit2DBlockWriteCallInst(llvm::CallInst &I, llvm::StringRef unmangledName);
  void visitPrefetchCallInst(llvm::CallInst &I);
  void visit1DBlockReadCallInst(llvm::CallInst &I);
  void visit1DBlockWriteCallInst(llvm::CallInst &I);
  void visit1DBlockPrefetchCallInst(llvm::CallInst &I);
  void visitOCL1DBlockPrefetchCallInst(llvm::CallInst &I, llvm::SmallVectorImpl<llvm::StringRef> &Matches);
  void visitPredicatedLoadInst(llvm::CallInst &I);
  void visitPredicatedStoreInst(llvm::CallInst &I);
  void visitSpirvSqrtCallInst(llvm::CallInst &I);

private:
  llvm::Module *m_Module = nullptr;
  CodeGenContext *m_pCtx = nullptr;
  ModuleMetaData *m_Metadata = nullptr;
  bool m_changed = false;
  llvm::DenseSet<llvm::Function *> m_BuiltinsToRemove;

  void handleInstructionsDecorations();
  void handleGlobalVariablesDecorations();

  void handleHostAccessIntel(llvm::GlobalVariable &globalVariable, llvm::MDNode *node);
  template <typename T>
  void handleCacheControlINTEL(llvm::Instruction &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes);
  template <typename T>
  void handleCacheControlINTELFor2DBlockIO(llvm::CallInst &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes,
                                           llvm::StringRef unmangledName);
  void handleCacheControlINTELForPrefetch(llvm::CallInst &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes);
  template <typename T>
  void handleCacheControlINTELFor1DBlockIO(llvm::CallInst &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes);
  void handleCacheControlINTELForOCL1DBlockPrefetch(llvm::CallInst &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes,
                                                    llvm::SmallVectorImpl<llvm::StringRef> &Matches);
  void handleFPRoundingMode(llvm::Instruction &I, llvm::SmallPtrSetImpl<llvm::MDNode *> &MDNodes);

  llvm::Type *getArgumentType(std::string demangledName, size_t argNo, llvm::LLVMContext &ctx);
};

// Legacy Pass Manager wrapper.
class HandleSpirvDecorationMetadataLPM : public llvm::ModulePass {
public:
  static char ID;

  HandleSpirvDecorationMetadataLPM();
  ~HandleSpirvDecorationMetadataLPM() {}

  llvm::StringRef getPassName() const override { return HandleSpirvDecorationMetadata::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  bool runOnModule(llvm::Module &M) override;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper.
class HandleSpirvDecorationMetadataNPM : public llvm::PassInfoMixin<HandleSpirvDecorationMetadataNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-handle-spirv-decoration-metadata"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
