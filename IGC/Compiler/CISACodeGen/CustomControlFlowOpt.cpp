/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/CustomControlFlowOpt.hpp"

using namespace llvm;
using namespace IGC;

namespace {

class CustomControlFlowOpt : public llvm::FunctionPass {
public:
  static char ID; // Pass identification

  CustomControlFlowOpt() : FunctionPass(ID) {
    IGC::initializeCustomControlFlowOptPass(*PassRegistry::getPassRegistry());
  }

  virtual bool runOnFunction(llvm::Function &F) override;

private:
  bool fixFunction(llvm::Function &F);
  bool fixBasicBlock(BasicBlock *BB);
  void addFlowControl(SmallVector<Instruction *, 8> V, SelectInst *Sel,
                      BasicBlock *BB);
  bool containsCondition(SmallVector<Instruction *, 8> *V,
                         Instruction *Condition);
};
} // namespace

FunctionPass *IGC::createCustomControlFlowOptPass() {
  return new CustomControlFlowOpt();
}

char CustomControlFlowOpt::ID = 0;

namespace IGC {
// Register pass to igc-opt
#define PASS_FLAG "custom-control-flow-opt"
#define PASS_DESCRIPTION "Custom control flow optimization after CFG pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(CustomControlFlowOpt, PASS_FLAG, PASS_DESCRIPTION,
                          PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(PostDominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
IGC_INITIALIZE_PASS_END(CustomControlFlowOpt, PASS_FLAG, PASS_DESCRIPTION,
                        PASS_CFG_ONLY, PASS_ANALYSIS)
} // namespace IGC

bool CustomControlFlowOpt::runOnFunction(Function &F) {
  bool Modified = fixFunction(F);
  return Modified;
}

bool CustomControlFlowOpt::fixFunction(llvm::Function &F) {
  bool Modified = false;
  for (Function::iterator It = F.begin(), Be = F.end(); It != Be; ++It) {
    Modified |= fixBasicBlock(&*It);
  }
  return Modified;
}

bool CustomControlFlowOpt::fixBasicBlock(BasicBlock *BB) {
  bool Modified = false;
  for (auto It = BB->begin(), Be = BB->end(); It != Be; ++It) {
    bool IsFixable = false;
    Instruction *I = &(*It);
    SmallVector<Instruction *, 8> V;

    while (I->hasOneUse()) {
      // if in the chain of uses there is call instruction, it is optimal to add
      // flow control
      if (isa<CallInst>(I))
        IsFixable = true;

      V.push_back(I);
      Instruction *Child = dyn_cast<Instruction>(*(I->user_begin()));

      // if user is not an instruction or it is in different BB -> break
      if (!Child || Child->getParent() != I->getParent())
        break;
      I = Child;

      // we have found value that is used only by select
      if (SelectInst *SelInst = dyn_cast<SelectInst>(I)) {
        // if we found select, but there was not call instruction before ->
        // break
        if (!IsFixable)
          break;

        Instruction *Condition = dyn_cast<Instruction>(SelInst->getCondition());

        // if we have condition instruction in our use chain
        if (!Condition || containsCondition(&V, Condition))
          break;

        Modified = true;
        addFlowControl(V, SelInst, BB);
        It = BB->begin();
        break;
      }
    }
  }
  return Modified;
}

bool CustomControlFlowOpt::containsCondition(SmallVector<Instruction *, 8> *V,
                                             Instruction *Condition) {
  for (auto VecIt = V->begin(), VecEnd = V->end(); VecIt != VecEnd; ++VecIt)
    if ((*VecIt)->isIdenticalTo(Condition))
      return true;
  return false;
}

void CustomControlFlowOpt::addFlowControl(SmallVector<Instruction *, 8> V,
                                          SelectInst *Sel, BasicBlock *BB) {
  Instruction *First = *V.begin();
  auto &Context = First->getContext();
  BasicBlock *ThenBB =
      BasicBlock::Create(Context, "then", First->getParent()->getParent());

  int N = V.size();
  IGC_ASSERT_MESSAGE(N > 1, "Vector should have at least 2 elements");
  Instruction *InstUsedBySelect = V[N - 1];

  auto &List = ThenBB->getInstList();
  for (auto It = V.begin(), End = V.end(); It != End; ++It) {
    (*It)->removeFromParent();
    List.push_back(*It);
  }
  BasicBlock *NewBB = BB->splitBasicBlock(Sel, "continue");
  ThenBB->moveBefore(NewBB);

  BranchInst *BIPlaceholder = dyn_cast<BranchInst>(BB->getTerminator());
  IGC_ASSERT_MESSAGE(BIPlaceholder,
                     "Last instruction should be branch instruction");

  ReplaceInstWithInst(BIPlaceholder,
                      BranchInst::Create(ThenBB, NewBB, Sel->getCondition()));

  BranchInst *BI = BranchInst::Create(NewBB);
  List.push_back(BI);
  PHINode *Phi = PHINode::Create(Sel->getType(), 2);

  Phi->addIncoming(InstUsedBySelect, ThenBB);
  Phi->addIncoming(Sel->getTrueValue() != InstUsedBySelect
                       ? Sel->getTrueValue()
                       : Sel->getFalseValue(),
                   BB);

  ReplaceInstWithInst(Sel, Phi);
}
