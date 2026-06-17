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
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include <memory>
#include "llvmWrapper/IR/Module.h"

namespace IGC {
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class BufferBoundsChecking : public llvm::InstVisitor<BufferBoundsChecking> {
public:
  BufferBoundsChecking() {}
  ~BufferBoundsChecking() = default;

  static llvm::StringRef getPassName() { return "BufferBoundsChecking"; }

  bool run(llvm::Module &M, IGCMD::MetaDataUtils *pMdUtils, ModuleMetaData *pModMD, CodeGenContext *pCtx);

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
  std::unique_ptr<KernelArgs> kernelArgs;
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

// Legacy Pass Manager wrapper.
class BufferBoundsCheckingLPM : public llvm::ModulePass {
public:
  static char ID;

  BufferBoundsCheckingLPM();
  ~BufferBoundsCheckingLPM() = default;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &analysisUsage) const override {
    analysisUsage.addRequired<MetaDataUtilsWrapper>();
    analysisUsage.addRequired<CodeGenContextWrapper>();
  }

  virtual llvm::StringRef getPassName() const override { return BufferBoundsChecking::getPassName(); }

  virtual bool runOnModule(llvm::Module &M) override {
    auto &MDUWAnalysis = getAnalysis<MetaDataUtilsWrapper>();
    return m_impl.run(M, MDUWAnalysis.getMetaDataUtils(), MDUWAnalysis.getModuleMetaData(),
                      getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }

private:
  BufferBoundsChecking m_impl;
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class BufferBoundsCheckingNPM : public llvm::PassInfoMixin<BufferBoundsCheckingNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-buffer-bounds-checking"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
