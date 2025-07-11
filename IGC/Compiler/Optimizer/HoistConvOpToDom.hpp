/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Dominators.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"

namespace IGC {
// This pass should be run after GVN. It searches for the patterns, where
// 1. There is a PHI instruction with the "same" incoming value from all
// incoming blocks
//  - Which means there is common dominator for all predecessors which define
//  the incoming values
// 2. Incoming value is limited for now to conversion operations, but can be
// extended in the future, if needed. Patterns like that are created by GVN's
// PRE and are exposed, when gid is used as an index for load and store. Source
// example:
//     val = buffer[gid];
//     outputBuffer[gid] = val;
// IR example:
//     entry:
//       %4 = add i32 %3, %payloadHeader.scalar
//       br i1 %6, label %if.then, label %entry.if.end_crit_edge
//     entry.if.end_crit_edge:
//       %.pre = sext i32 %4 to i64
//       br label %if.end
//     if.then:
//       %idxprom2 = sext i32 %4 to i64
//       load
//       br label %if.end
//     if.end:
//       %idxprom4.pre-phi = phi i64 [ %.pre, %entry.if.end_crit_edge ], [
//       %idxprom2, %if.then ] store
//
// If such pattern is found, optimization:
// 1. Hoists the value '%.pre' to the common dominator
// 2. Replaces all uses of '%idxprom2' with '%.pre'
// 3. Replaces all uses of '%idxprom4.pre-phi' with '%.pre'
// 4. Removes '%idxprom2' and '%idxprom4.pre-phi'
// Result should be:
//     entry:
//       %4 = add i32 %3, %payloadHeader.scalar
//       %.pre = sext i32 %4 to i64
//       br i1 %6, label %if.then, label %entry.if.end_crit_edge
//     entry.if.end_crit_edge:
//       br label %if.end
//     if.then:
//       load
//       br label %if.end
//     if.end:
//       store
// Then, SimplifyCFG is expected to run to remove `entry.if.end_crit_edge` block
// Then PromoteToPredicatedMemoryAccess pass can try to convert memory
// operations to predicated memory operations

class HoistConvOpToDom : public llvm::FunctionPass {
public:
  static char ID;
  explicit HoistConvOpToDom();
  ~HoistConvOpToDom() {}

  llvm::StringRef getPassName() const override { return "HoistConvOpToDom"; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<llvm::DominatorTreeWrapperPass>();
  }

  bool runOnFunction(llvm::Function &F) override;

private:
  llvm::BasicBlock *findNearestCommonDominator(const llvm::PHINode &PHI) const;

  CodeGenContext *m_CGCtx = nullptr;
  llvm::DominatorTree *m_DT = nullptr;
};
} // namespace IGC
