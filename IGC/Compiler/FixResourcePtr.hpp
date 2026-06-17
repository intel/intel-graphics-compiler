/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// @brief  This pass fixes the usage of GetBufferPtr, remove the combination of GetBufferPtr and GetElementPtr
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class FixResourcePtr {
public:
  FixResourcePtr() {}
  ~FixResourcePtr() {}

  static llvm::StringRef getPassName() { return "FixResourcePtrPass"; }

  /// @brief  Main entry point.
  /// @param  F The destination function.
  bool run(llvm::Function &F);

private:
  llvm::Value *ResolveBufferIndex(llvm::Value *bufferIndex, llvm::Value *vectorIndex = nullptr);

  void RemoveGetBufferPtr(llvm::GenIntrinsicInst *bufPtr, llvm::Value *bufIdx);

  void FindGetElementPtr(llvm::Instruction *bufPtr, llvm::Instruction *searchPtr);

  void FindLoadStore(llvm::Instruction *bufPtr, llvm::Instruction *eltPtr, llvm::Instruction *searchPtr);

  llvm::Value *GetByteOffset(llvm::Instruction *eltPtr);

  llvm::Value *CreateLoadIntrinsic(llvm::LoadInst *inst, llvm::Instruction *bufPtr, llvm::Value *offsetVal);

  llvm::Value *CreateStoreIntrinsic(llvm::StoreInst *inst, llvm::Instruction *bufPtr, llvm::Value *offsetVal);

  /// @brief  Indicates if the pass changed the processed function
  bool m_changed;
  /// Function we are processing
  llvm::Function *curFunc;
  /// Need data-layout for fixing pointer
  const llvm::DataLayout *DL;
  /// agent to modify the llvm-ir
  llvm::IRBuilder<> *builder;
  /// list of clean up after change
  std::vector<llvm::Instruction *> eraseList;
};

// Legacy Pass Manager wrapper.
class FixResourcePtrLPM : public llvm::FunctionPass {
public:
  /// Pass identification, replacement for typeid
  static char ID;

  FixResourcePtrLPM();
  ~FixResourcePtrLPM() {}

  virtual llvm::StringRef getPassName() const override { return FixResourcePtr::getPassName(); }

  virtual bool runOnFunction(llvm::Function &F) override { return FixResourcePtr().run(F); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { IGC_UNUSED(AU); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. No analysis dependencies, so a plain function pass. name()
// returns the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class FixResourcePtrNPM : public llvm::PassInfoMixin<FixResourcePtrNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-fix-resource-ptr"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
