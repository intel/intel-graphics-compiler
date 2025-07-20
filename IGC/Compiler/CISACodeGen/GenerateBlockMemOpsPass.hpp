/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/ScalarEvolution.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "AdaptorCommon/ImplicitArgs.hpp"

namespace IGC {
class GenerateBlockMemOpsPass : public llvm::FunctionPass {
public:
  static char ID;

  GenerateBlockMemOpsPass();

  virtual llvm::StringRef getPassName() const override { return "Generate block memory operations"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addRequired<llvm::ScalarEvolutionWrapperPass>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<WIAnalysis>();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  llvm::Value *checkGep(llvm::Instruction *Gep);
  bool isLocalIdX(const llvm::Value *InputVal);
  bool isR0(const llvm::Value *InputVal);
  bool isDataTypeSupported(llvm::Value *Ptr, llvm::Type *DataType);
  bool isIndexContinuous(llvm::Value *Addr);
  bool checkVectorizationAlongX(llvm::Function *F);
  bool checkLoopPhiVals(llvm::Loop *L);
  bool changeToBlockInst(llvm::Instruction *I);
  bool doesLoopHaveExternUse(llvm::Loop *L);
  bool getOffset(llvm::Value *Init, llvm::SmallVector<llvm::Value *, 2> &Offset);
  bool canOptLoadStore(llvm::Instruction *I);
  bool isLoopPattern(llvm::Loop *L);
  void setAlignmentAttr(llvm::CallInst *CI, const unsigned &Alignment);

  WIAnalysis *WI = nullptr;
  IGC::CodeGenContext *CGCtx = nullptr;
  IGC::IGCMD::MetaDataUtils *MdUtils = nullptr;
  llvm::DominatorTree *DT = nullptr;
  llvm::LoopInfo *LI;
  llvm::ScalarEvolution *SE;
  size_t SimdSize = 0;
};
} // namespace IGC