/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/WIAnalysis.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;

namespace IGC {

struct PropagateCmpUniformity : public FunctionPass {
  static char ID;
  WIAnalysis *m_WI = nullptr;

  PropagateCmpUniformity();

  bool runOnFunction(Function &F) override;

private:
  bool getEqualityBranches(CmpInst *cmp, BranchInst *br, BasicBlock *&trueBranch, BasicBlock *&falseBranch,
                           Value *&uniform, Value *&nonUniform);
  bool getUniformNonUniformPair(Value *op0, Value *op1, Value *&uniform, Value *&nonUniform);
  bool canReplaceUse(Use &U, BasicBlock *trueBranch, BasicBlock *cmpBB, DominatorTree &DT);
  bool replaceNonUniformWithUniform(CmpInst *cmp, Value *nonUniform, Value *uniform, BasicBlock *trueBranch,
                                    BasicBlock *cmpBB, DominatorTree &DT);

public:
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};
FunctionPass *createPropagateCmpUniformityPass();
} // namespace IGC
