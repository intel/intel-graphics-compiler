/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CodeGenPublic.h"

#include <string>

namespace IGC {
class HandleSpirvDecorationMetadata : public llvm::ModulePass, public llvm::InstVisitor<HandleSpirvDecorationMetadata> {
public:
  static char ID;

  HandleSpirvDecorationMetadata();
  ~HandleSpirvDecorationMetadata() {}

  virtual llvm::StringRef getPassName() const override { return "HandleSpirvDecorationMetadata"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
  }

  virtual bool runOnModule(llvm::Module &F) override;

  void visitLoadInst(llvm::LoadInst &I);
  void visitStoreInst(llvm::StoreInst &I);
  void visitCallInst(llvm::CallInst &I);
  void visit2DBlockReadCallInst(llvm::CallInst &I, llvm::StringRef unmangledName);
  void visit2DBlockWriteCallInst(llvm::CallInst &I, llvm::StringRef unmangledName);
  void visitPrefetchCallInst(llvm::CallInst &I);
  void visit1DBlockReadCallInst(llvm::CallInst &I);
  void visit1DBlockWriteCallInst(llvm::CallInst &I);
  void visit1DBlockPrefetchCallInst(llvm::CallInst &I);
  void visitOCL1DBlockPrefetchCallInst(llvm::CallInst &I, llvm::SmallVectorImpl<llvm::StringRef> &Matches);
  void visitPredicatedLoadInst(llvm::CallInst &I);
  void visitPredicatedStoreInst(llvm::CallInst &I);

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

  llvm::Type *getArgumentType(std::string demangledName, size_t argNo, llvm::LLVMContext &ctx);
};
} // namespace IGC
