/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef GENXGLOBALUNIFORM_H
#define GENXGLOBALUNIFORM_H

#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/DenseSet.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/InstrTypes.h"

namespace llvm {

class GenXGlobalUniformAnalysis : public FunctionPass {
public:
  static char ID;

  explicit GenXGlobalUniformAnalysis() : FunctionPass(ID) {}

  StringRef getPassName() const override {
    return "GenX global uniform analysis";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;
  void releaseMemory() override { m_Divergent.clear(); }

  void print(raw_ostream &OS) const;

  bool isUniform(BasicBlock &BB) const {
    IGC_ASSERT(BB.getParent() == F);
    return m_Divergent.find(&BB) == m_Divergent.end();
  }


private:
  Function *F = nullptr;
  PostDominatorTree *PDT = nullptr;

  DenseSet<BasicBlock *> m_Divergent;

  void analyzeDivergentCFG(IGCLLVM::TerminatorInst *Inst);
};

void initializeGenXGlobalUniformAnalysisPass(PassRegistry &);

} // end namespace llvm

#endif // GENXGLOBALUNIFORM_H
