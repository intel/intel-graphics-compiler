/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include "llvm/ADT/StringSwitch.h"
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include <llvmWrapper/IR/Instructions.h>
#include <llvmWrapper/IR/IRBuilder.h>

namespace IGC {

class WorkItem {
public:
  llvm::AllocaInst *Alloca;
  llvm::CallInst *MemSetCall;
  llvm::BitCastInst *Bitcast;
  llvm::CallInst *MemCpyCall;
  llvm::Value *Destination;
  llvm::MaybeAlign Alignment;
  uint64_t AllocaArraySize;
  bool IsVolatile;

  WorkItem(llvm::AllocaInst *alloca, llvm::Value *destination, llvm::CallInst *memsetCall, llvm::BitCastInst *bitcast,
           llvm::CallInst *memCpyCall, llvm::MaybeAlign alignment, uint64_t allocaArraySize, bool isVolatile) {
    IGC_ASSERT_MESSAGE(alloca, "alloca cannot be NULLPTR");
    IGC_ASSERT_MESSAGE(destination, "destination cannot be NULLPTR");
    IGC_ASSERT_MESSAGE(memsetCall, "memSetCall cannot be NULLPTR");
    IGC_ASSERT_MESSAGE(bitcast, "bitcast cannot be NULLPTR");
    IGC_ASSERT_MESSAGE(memCpyCall, "memCpyCall cannot be NULLPTR");
    Alloca = alloca;
    Destination = destination;
    MemSetCall = memsetCall;
    Bitcast = bitcast;
    MemCpyCall = memCpyCall;
    Alignment = alignment;
    AllocaArraySize = allocaArraySize;
    IsVolatile = isVolatile;
  }
};

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class FoldZeroInitAllocaIntoMemset {
public:
  FoldZeroInitAllocaIntoMemset() {}

  static llvm::StringRef getPassName() { return "FoldZeroInitAllocaIntoMemset"; }

  bool runOnFunction(llvm::Function &F);

private:
  std::vector<WorkItem> findUnnecessaryAllocaInstances(llvm::Function &F);
};

// Legacy Pass Manager wrapper.
class FoldZeroInitAllocaIntoMemsetLPM : public llvm::FunctionPass {
public:
  static char ID;

  FoldZeroInitAllocaIntoMemsetLPM();
  ~FoldZeroInitAllocaIntoMemsetLPM() {}

  virtual llvm::StringRef getPassName() const override { return FoldZeroInitAllocaIntoMemset::getPassName(); }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {}

  virtual bool runOnFunction(llvm::Function &F) override { return m_impl.runOnFunction(F); }

private:
  FoldZeroInitAllocaIntoMemset m_impl;
};

class FoldZeroInitAllocaIntoMemsetNPM : public llvm::PassInfoMixin<FoldZeroInitAllocaIntoMemsetNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-fold-zeroinit-alloca-into-memset"; }
  static bool isRequired() { return true; }
};
} // namespace IGC
