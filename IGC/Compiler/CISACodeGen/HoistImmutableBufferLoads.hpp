/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// Lifts loads from read-only buffers out of loops that LICM leaves them in,
/// for example because the loop holds an atomic, which MemorySSA counts as a
/// clobber of the load.
///
/// A lifted value stays live across the whole loop, and ConstantCoalescing
/// later merges neighbouring lifted loads into wider ones. A loop with more
/// distinct candidates than HoistImmutableBufferLoadsMaxPerLoop keeps all of
/// them.
class HoistImmutableBufferLoads : public llvm::FunctionPass {
public:
  static char ID;

  HoistImmutableBufferLoads();

  llvm::StringRef getPassName() const override { return "HoistImmutableBufferLoads"; }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<llvm::LoopInfoWrapperPass>();
    AU.addPreserved<llvm::DominatorTreeWrapperPass>();
    AU.addPreserved<llvm::LoopInfoWrapperPass>();
  }

  bool runOnFunction(llvm::Function &F) override;
};

llvm::FunctionPass *createHoistImmutableBufferLoadsPass();
} // namespace IGC
