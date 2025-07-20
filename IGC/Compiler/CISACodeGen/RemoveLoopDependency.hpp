/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
// This pass is designed to remove fantom dependency in a loop.
// When value that we get from PHI, is a vector, that is completely overwritten
// inside loop body and no other uses, then replace this value with usage of undef.
// Input IR:
//      %1 = phi <4 x i32> [ undef, %for.body9.i ], [ %.124.vec.insert1133, %._crit_edge646.3 ]
//      %2 = insertelement <4 x i32> %1, i32 %val1, i32 0
//      %3 = insertelement <4 x i32> %2, i32 %val2, i32 1
//      %4 = insertelement <4 x i32> %3, i32 %val3, i32 2
//      %5 = insertelement <4 x i32> %4, i32 %val4, i32 3
//
// Output IR:
//      %1 = phi <4 x i32> [ undef, %for.body9.i ], [ %.124.vec.insert1133, %._crit_edge646.3 ]
//      %2 = insertelement <4 x i32> undef, i32 %val1, i32 0
//      %3 = insertelement <4 x i32> %2, i32 %val2, i32 1
//      %4 = insertelement <4 x i32> %3, i32 %val3, i32 2
//      %5 = insertelement <4 x i32> %4, i32 %val4, i32 3

class RemoveLoopDependency : public llvm::FunctionPass {

public:
  static char ID;
  RemoveLoopDependency();

  virtual llvm::StringRef getPassName() const override { return "RemoveLoopDependency"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<llvm::LoopInfoWrapperPass>(); }

  virtual bool runOnFunction(llvm::Function &F) override;
  bool processLoop(llvm::Loop *L);
  bool RemoveDependency(llvm::PHINode *PHI, llvm::Loop *L);
};
} // namespace IGC
