/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

// DiamondChainMergePass is built against LLVM 22 only.
#if LLVM_VERSION_MAJOR == 22

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CFG.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/IntrinsicInst.h>
#include <llvm/Transforms/Utils/ValueMapper.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

class DiamondChainMergePass : public llvm::FunctionPass {
public:
  static char ID;

  DiamondChainMergePass();

  llvm::StringRef getPassName() const override { return "DiamondChainMergePass"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
  }

  bool runOnFunction(llvm::Function &F) override;

public:
  struct MergeCandidate {
    llvm::BasicBlock *DstTrue = nullptr;
    llvm::BasicBlock *DstFalse = nullptr;
    llvm::BasicBlock *DstMergeBB = nullptr;
    llvm::SmallVector<llvm::BasicBlock *, 8> TrueBlocks;
    llvm::SmallVector<llvm::BasicBlock *, 8> FalseBlocks;
    llvm::SmallVector<llvm::BasicBlock *, 8> MergeBlocks;
    llvm::SmallVector<bool, 8> FlipDstForTriple;
  };

  static bool hasAnyBarrierLikeIntrinsic(llvm::BasicBlock *BB);
  static bool hasAnyConvergentInst(llvm::BasicBlock *BB);

private:
  static bool isInvalidGroupingCond(llvm::Value *Cond);
  static bool isValidTriangleSink(llvm::BasicBlock *BB, llvm::BasicBlock *BodyBB);
  static llvm::BasicBlock *getUncondSucc(llvm::BasicBlock *BB);
  static void trimCandidateToPrefix(MergeCandidate &C, size_t SafeCount);

  bool mergeSourceTriple(const MergeCandidate &C, size_t I) const;
  bool mergeCandidate(MergeCandidate &C);
  bool cloneIntoDestination(llvm::BasicBlock *Dst, llvm::BasicBlock *Src, llvm::ValueToValueMapTy &Map,
                            llvm::SmallVectorImpl<llvm::BasicBlock *> &BlocksToRemove);
  bool cloneMergeBlock(llvm::BasicBlock *DstTrue, llvm::BasicBlock *DstFalse, llvm::BasicBlock *MergeBB,
                       llvm::ValueToValueMapTy &TrueMap, llvm::ValueToValueMapTy &FalseMap,
                       llvm::SmallVectorImpl<llvm::BasicBlock *> &BlocksToRemove);
};

} // namespace IGC

#endif // LLVM_VERSION_MAJOR == 22
