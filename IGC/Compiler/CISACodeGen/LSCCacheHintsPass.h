/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp" // for suppressing LLVM warnings
#include <llvm/ADT/StringRef.h>        // for llvm::StringRef
#include <llvm/IR/Function.h>          // for llvm::Function
#include <llvm/Pass.h>                 // for llvm::FunctionPass
#include "common/LLVMWarningsPop.hpp"  // for suppressing LLVM warnings

namespace IGC {


class LSCCacheHints : public llvm::FunctionPass {
public:
  LSCCacheHints();
  llvm::StringRef getPassName() const override { return "LSCCacheHints"; }
  bool runOnFunction(llvm::Function &F) override;
  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;

  static char ID;

private:
  void VisitInstruction(const ModuleMetaData *ModMD, llvm::Instruction &I);
  void TranslateLscCacheCtrlToPisaCacheCtrl(llvm::Instruction &I) const;
  void SetInstructionCacheHint(llvm::Instruction &I, const unsigned CacheHint, llvm::StringRef Reason) const;
  void SetupLscCacheCtrl(const ModuleMetaData *ModMD, llvm::Instruction &I);
  bool ShouldGenerateLSC_Duplicate(llvm::Instruction &I, bool isTGM) const;
  uint32_t TotalBytesToStoreOrLoad_Duplicate(llvm::Instruction *vectorLdStInst) const;
  unsigned int GetScalarTypeSizeInRegister_Duplicate(const llvm::Type *Ty) const;
  bool UseRasterizerOrderedByteAddressBuffer(const ModuleMetaData *ModMD, llvm::Instruction &I) const;
};

void initializeLSCCacheHintsPass(llvm::PassRegistry &);
llvm::FunctionPass *createLSCCacheHintsPass();

} // namespace IGC
