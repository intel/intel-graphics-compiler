/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "AdaptorCommon/ImplicitArgs.hpp"
#include "Compiler/Optimizer/OpenCLPasses/KernelArgs/KernelArgs.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/IRBuilder.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/Module.h"

namespace IGC {
class BufferBoundsChecking : public llvm::ModulePass, public llvm::InstVisitor<BufferBoundsChecking> {
public:
  static char ID;

  BufferBoundsChecking();
  ~BufferBoundsChecking() = default;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &analysisUsage) const override {
    analysisUsage.addRequired<MetaDataUtilsWrapper>();
    analysisUsage.addRequired<CodeGenContextWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return "BufferBoundsChecking"; }

  virtual bool runOnModule(llvm::Module &M) override;

  void visitLoadInst(llvm::LoadInst &load);
  void visitStoreInst(llvm::StoreInst &store);

private:
  struct AccessInfo {
    llvm::StringRef filename;
    int line;
    int column;
    llvm::StringRef bufferName;
    llvm::Value *bufferAddress;
    llvm::Value *bufferOffsetInBytes;
    uint32_t implicitArgBufferSizeIndex;
    llvm::Value *elementSizeInBytes;
  };

  bool modified = false;

  IGCMD::MetaDataUtils *metadataUtils = nullptr;
  ModuleMetaData *moduleMetadata = nullptr;
  ImplicitArgs *implicitArgs = nullptr;
  KernelArgs *kernelArgs = nullptr;
  llvm::SmallVector<llvm::Instruction *, 4> loadsAndStoresToCheck;
  llvm::DenseMap<llvm::StringRef, llvm::GlobalVariable *> stringsCache;
  llvm::DICompileUnit *compileUnit = nullptr;
  llvm::Function *bufferSizePlaceholderFunction = nullptr;
  llvm::Value *localId0;
  llvm::Value *localId1;
  llvm::Value *localId2;
  llvm::Value *globalId0;
  llvm::Value *globalId1;
  llvm::Value *globalId2;

  std::tuple<llvm::Value *, llvm::Value *, llvm::Value *> getLocalIds(llvm::Function &function);
  std::tuple<llvm::Value *, llvm::Value *, llvm::Value *> getGlobalIds(llvm::Function &function);

  void handleLoadStore(llvm::Instruction *instruction);

  bool argumentQualifiesForChecking(const llvm::Argument *argument);

  llvm::Value *createBoundsCheckingCondition(const AccessInfo &accessInfo, llvm::Instruction *insertBefore);
  llvm::Value *createLoadStoreReplacement(llvm::Instruction *instruction, llvm::Instruction *insertBefore);
  void createAssertCall(const AccessInfo &accessInfo, llvm::Instruction *insertBefore);
  llvm::SmallVector<llvm::Value *, 4> createAssertArgs(const AccessInfo &accessInfo, llvm::Instruction *insertBefore);
  llvm::GlobalVariable *getOrCreateGlobalConstantString(llvm::Module *M, llvm::StringRef format);
  void createBoundsCheckingCode(llvm::Instruction *instruction, const AccessInfo &accessInfo);

  const IGC::KernelArg *getKernelArg(llvm::Value *value);
  const IGC::KernelArg *getKernelArgFromPtr(const llvm::PointerType &pointerType, llvm::Value *value);
  llvm::Argument *getBufferSizeArg(llvm::Function *function, uint32_t n);
  AccessInfo getAccessInfo(llvm::Instruction *instruction, llvm::Value *value);

  llvm::Value *createBufferSizePlaceholder(uint32_t implicitArgBufferSizeIndex, llvm::Instruction *insertBefore);
};
} // namespace IGC
