/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "GenXGlobalUniform.h"
#include "GenXUtil.h"
#include "GenX.h"

#include "llvm/IR/InstIterator.h"
#include "llvm/InitializePasses.h"

#include <stack>

#define DEBUG_TYPE "GENX_GLOBAL_UNIFORM"

using namespace llvm;

static cl::opt<bool>
    PrintGlobalUniform("print-global-uniform-info", cl::init(false), cl::Hidden,
                       cl::desc("Print GenXGlobalUniform analysis results"));

char GenXGlobalUniformAnalysis::ID = 0;

INITIALIZE_PASS_BEGIN(GenXGlobalUniformAnalysis, "GenXGlobalUniformAnalysis",
                      "GenXGlobalUniformAnalysis", false, true)
INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
INITIALIZE_PASS_END(GenXGlobalUniformAnalysis, "GenXGlobalUniformAnalysis",
                    "GenXGlobalUniformAnalysis", false, true)

FunctionPass *llvm::createGenXGlobalUniformAnalysisPass() {
  initializeGenXGlobalUniformAnalysisPass(*PassRegistry::getPassRegistry());
  return new GenXGlobalUniformAnalysis();
}

void GenXGlobalUniformAnalysis::getAnalysisUsage(AnalysisUsage &AU) const {
  FunctionPass::getAnalysisUsage(AU);
  AU.addRequired<PostDominatorTreeWrapperPass>();
  AU.setPreservesCFG();
}

void GenXGlobalUniformAnalysis::print(raw_ostream &OS) const {
  OS << "Non-uniform basic blocks:\n";
  for (auto *BB : m_Divergent)
    OS << "\t" << BB->getName() << "\n";
}

void GenXGlobalUniformAnalysis::analyzeDivergentCFG(
    IGCLLVM::TerminatorInst *TI) {
  BasicBlock *StartBB = TI->getParent();
  if (m_Divergent.find(StartBB) != m_Divergent.end())
    return;

  // Get immediate post dominator - the basic block where CFG joins again.
  // It could be null in case when the function has multiple exits
  BasicBlock *IPD = PDT->getNode(StartBB)->getIDom()->getBlock();

  std::stack<BasicBlock *> Stack;
  Stack.push(StartBB);

  DenseSet<BasicBlock *> Visited;

  // Mark as divergent all the basic blocks from the start till the IPD
  while (!Stack.empty()) {
    auto *BB = Stack.top();
    Stack.pop();
    Visited.insert(BB);
    for (auto *Succ : successors(BB)) {
      if (Succ == IPD)
        continue;
      if (Visited.find(Succ) != Visited.end())
        continue;
      m_Divergent.insert(Succ);
      Stack.push(Succ);
    }
  }
}

bool GenXGlobalUniformAnalysis::runOnFunction(Function &F) {
  this->F = &F;
  PDT = &getAnalysis<PostDominatorTreeWrapperPass>().getPostDomTree();

  std::stack<Value *> Stack;

  // Find all the non-uniform values in function
  // First check the arguments
  for (auto &Arg : F.args()) {
    if (Arg.getName() == "impl.arg.llvm.genx.local.id16" ||
        Arg.getName() == "impl.arg.llvm.genx.local.id")
      Stack.push(cast<Value>(&Arg));
  }

  // Do the initial instrucion scan
  for (auto &Inst : instructions(F)) {
    unsigned IID = vc::getAnyIntrinsicID(&Inst);
    switch (IID) {
    case GenXIntrinsic::genx_group_id_x:
    case GenXIntrinsic::genx_group_id_y:
    case GenXIntrinsic::genx_group_id_z:
    case GenXIntrinsic::genx_local_id:
    case GenXIntrinsic::genx_local_id16:
      Stack.push(cast<Value>(&Inst));
      break;
    }
  }

  DenseSet<Value *> Visited;

  // Traverse the use-def chain for found non-uniform values
  // and mark every user of non-uniform value also as non-uniform
  while (!Stack.empty()) {
    auto *V = Stack.top();
    Stack.pop();
    Visited.insert(V);
    for (auto *U : V->users()) {
      if (Visited.find(cast<Value>(U)) != Visited.end())
        continue;
      auto *Inst = dyn_cast<Instruction>(U);
      if (Inst && Inst->isTerminator()) {
        // Non-uniform terminator found. This means the start of divergent CFG
        analyzeDivergentCFG(Inst);
      }
      Stack.push(cast<Value>(U));
    }
  }

  if (PrintGlobalUniform)
    print(outs());

  return false;
}
