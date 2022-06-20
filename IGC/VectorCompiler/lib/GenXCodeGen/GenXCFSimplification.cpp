/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXCFSimplification
/// --------------------
///
/// This is a function pass that simplifies CF as follows:
///
/// * Where a conditional branch on "not any(pred)" branches over a single
///   basic block containing a small number of instructions, and all
///   instructions are either predicated by pred or are used only in the same
///   basic block, then change the branch to "branch never" so it gets
///   removed later.
///
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXIntrinsics.h"
#include "GenXModule.h"
#include "llvm/Analysis/InstructionSimplify.h"
#include "llvm/GenXIntrinsics/GenXIntrinsics.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm-c/Core.h"

#define DEBUG_TYPE "GENX_CFSIMPLIFICATION"

using namespace llvm;
using namespace genx;

namespace {

// GenXCFSimplification : simplify SIMD CF code
class GenXCFSimplification : public FunctionPass {
  static const unsigned Threshold;
  bool Modified = false;
  SmallVector<BasicBlock *, 4> BranchedOver;
public:
  static char ID;
  explicit GenXCFSimplification() : FunctionPass(ID) { }
  StringRef getPassName() const override {
    return "GenX SIMD CF simplification";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

private:
  bool isBranchedOverBlock(BasicBlock *BB);
  BasicBlock *processBranchedOverBlock(BasicBlock *BB);
  bool isPredSubsetOf(Value *Pred1, Value *Pred2, bool Inverted);
};

// Threshold for removing a simd cf branch. The 9999 setting means it is
// pretty much always removed when it can be.
const unsigned GenXCFSimplification::Threshold = 9999;

} // end anonymous namespace

char GenXCFSimplification::ID = 0;
namespace llvm { void initializeGenXCFSimplificationPass(PassRegistry &); }
INITIALIZE_PASS_BEGIN(GenXCFSimplification, "GenXCFSimplification", "GenXCFSimplification", false, false)
INITIALIZE_PASS_END(GenXCFSimplification, "GenXCFSimplification", "GenXCFSimplification", false, false)

FunctionPass *llvm::createGenXCFSimplificationPass()
{
  initializeGenXCFSimplificationPass(*PassRegistry::getPassRegistry());
  return new GenXCFSimplification();
}

void GenXCFSimplification::getAnalysisUsage(AnalysisUsage &AU) const
{
}

/***********************************************************************
 * GenXCFSimplification::runOnFunction : process one function to
 *    simplify SIMD CF
 */
bool GenXCFSimplification::runOnFunction(Function &F)
{
  LLVM_DEBUG(dbgs() << "GenXCFSimplification::runOnFunction(" << F.getName() << ")\n");
  Modified = false;
  // Build a list of simple branched over basic blocks.
  for (auto fi = F.begin(), fe = F.end(); fi != fe; ++fi) {
    auto BB = &*fi;
    if (isBranchedOverBlock(BB)) {
      LLVM_DEBUG(dbgs() << "is branched over: " << BB->getName() << "\n");
      BranchedOver.push_back(BB);
    }
  }
  // Process each branched over block.
  while (!BranchedOver.empty()) {
    auto BB = BranchedOver.back();
    BranchedOver.pop_back();
    BasicBlock *SubsumedInto = processBranchedOverBlock(BB);
    if (!SubsumedInto)
      continue;
    Modified = true;
    // The joined together block may now be a simple branched over block.
    if (isBranchedOverBlock(SubsumedInto)) {
      LLVM_DEBUG(dbgs() << "is branched over: " << SubsumedInto->getName() << "\n");
      BranchedOver.push_back(SubsumedInto);
    }
  }
  return Modified;
}


/***********************************************************************
 * isBranchedOverBlock : detect whether a basic block is a simple branched
 * over block. It must have a single predecessor and a single successor,
 * and the predecessor must end in a conditional branch whose other
 * successor is our successor.
 */
bool GenXCFSimplification::isBranchedOverBlock(BasicBlock *BB)
{
  if (BB->use_empty())
    return false; // no predecessors
  if (!BB->hasOneUse())
    return false; // more than one predecessor
  auto Term = BB->getTerminator();
  if (Term->getNumSuccessors() != 1)
    return false; // not exactly one successor
  Use *U = &*BB->use_begin();
  auto PredBr = dyn_cast<BranchInst>(U->getUser());
  if (!PredBr || !PredBr->isConditional())
    return false; // predecessor is not conditional branch
  auto Succ = Term->getSuccessor(0);
  if (PredBr->getSuccessor(0) == BB) {
    if (PredBr->getSuccessor(1) != Succ)
      return false; // other cond branch successor is not our successor
  } else {
    if (PredBr->getSuccessor(0) != Succ)
      return false; // other cond branch successor is not our successor
  }
  return true;
}

/***********************************************************************
 * processBranchedOverBlock : process a branched over block
 *
 * Return:  0 if unchanged, else the basic block that BB has been subsumed into
 */
BasicBlock *GenXCFSimplification::processBranchedOverBlock(BasicBlock *BB)
{
  LLVM_DEBUG(dbgs() << "processBranchedOverBlock: " << BB->getName() << "\n");
  // Check that the condition to enter the branched over block is an any
  // of a predicate.
  auto PredBr = cast<BranchInst>(BB->use_begin()->getUser());
  auto Cond = PredBr->getCondition();
  bool Inverted = false;
  switch (GenXIntrinsic::getGenXIntrinsicID(Cond)) {
    case GenXIntrinsic::genx_any:
      if (PredBr->getSuccessor(0) != BB)
        return nullptr; // branch is the wrong way round
      break;
    case GenXIntrinsic::genx_all:
      if (PredBr->getSuccessor(1) != BB)
        return nullptr; // branch is the wrong way round
      Inverted = true;
      break;
    default:
      return nullptr; // condition not "any" or "all"
  }
  Cond = cast<Instruction>(Cond)->getOperand(0);
  LLVM_DEBUG(dbgs() << "branched over simd cf block: " << BB->getName() << " with Cond " << Cond->getName()
      << (Inverted ? " (inverted)" : "") << "\n"
      << "(source line of branch is " << PredBr->getDebugLoc().getLine() << "\n");
  // Check that each phi node in the successor has incomings related as
  // follows: the incoming from BB must be a chain of selects or predicated
  // wrregions where the ultimate original input is the other incoming, and
  // each predicate must be Cond (inverted if necessary), or a subset of it.
  // Also count the phi nodes that have different incomings for the two blocks,
  // and if that goes over the threshold give up.
  unsigned Count = 0;
  BasicBlock *Succ = BB->getTerminator()->getSuccessor(0);
  BasicBlock *Pred = PredBr->getParent();
  for (auto Inst = &Succ->front(); ; Inst = Inst->getNextNode()) {
    auto Phi = dyn_cast<PHINode>(Inst);
    if (!Phi)
      break;
    LLVM_DEBUG(dbgs() << "Phi " << *Phi << "\n");
    Value *V = Phi->getIncomingValueForBlock(BB);
    Value *Orig = Phi->getIncomingValueForBlock(Pred);
    LLVM_DEBUG(dbgs() << "V: " << *V << "\n"
        << "Orig: " << *Orig << "\n");
    if (V == Orig)
      continue;
    // Check for special case that Orig is constant 0 and V is the condition
    // input to any, thus we know that V is 0 if the branch over is taken.
    // Thus we can change Pred's incoming to the phi node to match BB's.  Not
    // doing this can result in the branch over not being removable if it is an
    // inner if..else..endif.
    if (auto C = dyn_cast<Constant>(Orig)) {
      if (C->isNullValue() && V == Cond) {
        Phi->setIncomingValue(Phi->getBasicBlockIndex(Pred), V);
        continue;
      }
    }
    // Normal check on for phi node.
    bool OK = false;
    for (;;) {
      LLVM_DEBUG(dbgs() << "  checking " << *V << "\n");
      if (V == Orig) {
        OK = true;
        break;
      }
      auto Inst = dyn_cast<Instruction>(V);
      if (!Inst)
        break;
      if (++Count > Threshold) {
        LLVM_DEBUG(dbgs() << "Over threshold\n");
        break;
      }
      if (isa<SelectInst>(Inst)) {
        if (!isPredSubsetOf(Inst->getOperand(0), Cond, Inverted))
          break;
        V = Inst->getOperand(2);
        continue;
      }
      if (!GenXIntrinsic::isWrRegion(Inst))
        break;
      if (!isPredSubsetOf(Inst->getOperand(
              GenXIntrinsic::GenXRegion::PredicateOperandNum), Cond, Inverted))
        break;
      V = Inst->getOperand(0);
    }
    if (!OK) {
      LLVM_DEBUG(dbgs() << "failed\n");
      return nullptr;
    }
    LLVM_DEBUG(dbgs() << "OK\n");
  }
  // Check that the block does not contain any calls or intrinsics with
  // side effects.
  for (auto bi = BB->begin(), be = BB->end(); bi != be; ++bi)
    if (auto CI = dyn_cast<CallInst>(&*bi)) {
      if (!GenXIntrinsic::isAnyNonTrivialIntrinsic(CI)) {
        LLVM_DEBUG(dbgs() << "contains call\n");
        return nullptr;
      }
      if (!CI->getCalledFunction()->doesNotAccessMemory()) {
        LLVM_DEBUG(dbgs() << "contains intrinsic with side effect\n");
        return nullptr;
      }
    }
  // We can now do the transformation.
  LLVM_DEBUG(dbgs() << "Transforming " << BB->getName() << "\n");
  // Move instructions from BB into the predecessor.
  for (;;) {
    auto Inst = &BB->front();
    if (Inst) {
      if (Inst->isTerminator())
        break;
      Inst->removeFromParent();
      Inst->insertBefore(PredBr);
    }
  }
  // In each phi node in the successor, change the incoming for the predecessor
  // to match the incoming for our BB, and remove the incoming for our BB.
  // If that would leave only one incoming, then remove the phi node.
  for (auto Inst = &Succ->front();; ) {
    auto Phi = dyn_cast<PHINode>(Inst);
    if (!Phi)
      break;
    auto Next = Inst->getNextNode();
    if (Phi->getNumIncomingValues() == 2) {
      // Having got rid of the phi, it is worth running instruction
      // simplification on each use. Specifically, this turns the
      // P3 = (P1 & P2) | (P1 & ~P2) at the endif of an if that
      // has an else into the simpler P1. Without that, an enclosing if
      // would never have its branch removed, because the use of the "or"
      // as a predicate stops us detecting that all predicates are a
      // subset of the branch condition.
      Value *V = Phi->getIncomingValueForBlock(BB);
      replaceAndRecursivelySimplify(Phi, V);
    } else {
      unsigned PredIdx = Phi->getBasicBlockIndex(Pred);
      unsigned BBIdx = Phi->getBasicBlockIndex(BB);
      Phi->setIncomingValue(PredIdx, Phi->getIncomingValue(BBIdx));
      Phi->removeIncomingValue(BBIdx);
    }
    Inst = Next;
  }
  // Change the predecessor to have an unconditional branch to the successor.
  auto NewBr = BranchInst::Create(Succ, PredBr);
  NewBr->takeName(PredBr);
  auto CondInst = dyn_cast<Instruction>(PredBr->getCondition());
  PredBr->eraseFromParent();
  if (CondInst && CondInst->use_empty())
    CondInst->eraseFromParent();
  // Remove the now empty and unreferenced BB.
  BB->eraseFromParent();
  // Merge Pred and Succ blocks.
  MergeBlockIntoPredecessor(Succ);
  return Pred;
}

/***********************************************************************
 * isPredSubsetOf : detect whether Pred1 is a subset of Pred2 (or of ~Pred2
 *    if Inverted is set)
 */
bool GenXCFSimplification::isPredSubsetOf(Value *Pred1, Value *Pred2,
      bool Inverted)
{
  if (Pred1 == Pred2 && !Inverted)
    return true;
  auto BO = dyn_cast<BinaryOperator>(Pred1);
  if (!BO)
    return false;
  if (BO->getOpcode() == Instruction::And)
    return isPredSubsetOf(BO->getOperand(0), Pred2, Inverted)
      || isPredSubsetOf(BO->getOperand(1), Pred2, Inverted);
  if (BO->getOpcode() == Instruction::Xor)
    if (auto C = dyn_cast<Constant>(BO->getOperand(1)))
      return BO->getOperand(0) == Pred2 && C->isAllOnesValue();
  return false;
}

