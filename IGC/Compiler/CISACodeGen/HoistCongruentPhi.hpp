/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class HoistCongruentPHI : public llvm::FunctionPass {
  llvm::DominatorTree *DT = nullptr;
  CodeGenContext *CTX = nullptr;

public:
  static char ID; // Pass identification

  HoistCongruentPHI();

  virtual bool runOnFunction(llvm::Function &F) override;

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();

    AU.addRequired<llvm::DominatorTreeWrapperPass>();
    AU.addRequired<CodeGenContextWrapper>();

    AU.addPreserved<llvm::DominatorTreeWrapperPass>();
  }

  // Read-only: would HoistCongruentPHI run on this function at all?
  // Mirrors the function-level gates in runOnFunction() (shader type,
  // DisableCodeSinking, CodeSinkingMinSize). The CodeSinkingMinSize check
  // walks every instruction in F, so callers that test many phis in one
  // function (e.g. WIAnalysis) should cache this verdict per function.
  static bool wouldRunOnFunction(const llvm::Function &F, const CodeGenContext *CTX);

  // Read-only: would hoistCongruentPhi() eliminate `phi`?
  // Mirrors the apply-time decision in hoistCongruentPhi(PHINode*) but
  // performs no IR mutation. Assumes the caller has already verified the
  // function-level gates via wouldRunOnFunction().
  static bool wouldHoist(const llvm::PHINode *phi, const llvm::DominatorTree &DT);

private:
  // try to hoist phi nodes with congruent incoming values
  typedef std::pair<llvm::Instruction *, llvm::Instruction *> InstPair;
  typedef llvm::SmallVector<llvm::Instruction *, 4> InstVec;

  static void appendIfNotExist(InstPair src, std::vector<InstPair> &instMap) {
    if (std::find(instMap.begin(), instMap.end(), src) == instMap.end()) {
      instMap.push_back(src);
    }
  }
  static void appendIfNotExist(InstVec &dst, llvm::Instruction *inst) {
    if (std::find(dst.begin(), dst.end(), inst) == dst.end()) {
      dst.push_back(inst);
    }
  }
  static void appendIfNotExist(InstVec &dst, InstVec &src) {
    for (auto *I : src) {
      appendIfNotExist(dst, I);
    }
  }

  // check if two values are congruent (derived from same values), and
  // record all intermediate results in vector.
  static bool checkCongruent(const llvm::DominatorTree &DT, std::vector<InstPair> &instMap, const InstPair &values,
                             InstVec &leaves, unsigned depth);

  /**
   * Detech phi with congruent incoming values, and try to hoist them to
   * dominator.  In some cases, GVN may leave code like this and increase
   * register pressure.
   */
  bool hoistCongruentPhi(llvm::PHINode *phi);
  bool hoistCongruentPhi(llvm::Function &F);
};

void initializeHoistCongruentPHIPass(llvm::PassRegistry &);
} // namespace IGC
