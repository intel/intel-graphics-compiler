/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPromotePredicate
/// --------------------
///
/// GenXPromotePredicate is an optimization pass that promotes vector operations
/// on predicates (n x i1) to operations on wider integer types (<n x i16>).
/// This often reduces flag register pressure and improves code quality.
///
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "GENX_PROMOTE_PREDICATE"

#include "GenX.h"
#include "GenXUtil.h"
#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"

#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace genx;

static cl::opt<unsigned> LogicOpsThreshold(
    "logical-ops-threshold", cl::init(5), cl::Hidden,
    cl::desc("Number of logical predicate operations to apply GRF promotion"));

STATISTIC(NumCollectedPredicateWebs, "Number of collected predicate webs");
STATISTIC(NumPromotedPredicateWebs, "Number of GRF-promoted predicate webs");

namespace {

class GenXPromotePredicate : public FunctionPass {
public:
  static char ID;
  GenXPromotePredicate() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override;
  StringRef getPassName() const override { return "GenXPromotePredicate"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }
};

} // namespace

char GenXPromotePredicate::ID = 0;

namespace llvm {
void initializeGenXPromotePredicatePass(PassRegistry &);
}
INITIALIZE_PASS_BEGIN(GenXPromotePredicate, "GenXPromotePredicate",
                      "GenXPromotePredicate", false, false)
INITIALIZE_PASS_END(GenXPromotePredicate, "GenXPromotePredicate",
                    "GenXPromotePredicate", false, false)

FunctionPass *llvm::createGenXPromotePredicatePass() {
  initializeGenXPromotePredicatePass(*PassRegistry::getPassRegistry());
  return new GenXPromotePredicate;
}

static constexpr unsigned PromotedPredicateWidth = 16;

// Get predicate value with grf type.
static Value *getExtendedValue(Value *Val, Instruction *InsertBefore) {
  IGC_ASSERT(Val->getType()->isIntOrIntVectorTy(1));
  IRBuilder<> IRB(InsertBefore);
  Type *NewTy =
      IGCLLVM::getWithNewBitWidth(Val->getType(), PromotedPredicateWidth);
  return IRB.CreateSExt(Val, NewTy, Val->getName() + ".widened");
}

// Get grf value with predicate type.
static Value *getTruncatedValue(Value *Val, Instruction *InsertBefore) {
  IGC_ASSERT(Val->getType()->isIntOrIntVectorTy(PromotedPredicateWidth));
  IRBuilder<> IRB(InsertBefore);
  Type *NewTy = IGCLLVM::getWithNewBitWidth(Val->getType(), 1);
  return IRB.CreateTrunc(Val, NewTy, Val->getName() + ".truncated");
}

// Promote one predicate instruction to grf - promote all its operands and
// instruction itself, and then sink the result back to predicate.
static Value *promoteInst(Instruction *Inst) {
  IRBuilder<> IRB(Inst);
  // Special case - phi node.
  if (auto *Phi = dyn_cast<PHINode>(Inst)) {
    auto *WidenedPhi = IRB.CreatePHI(
        IGCLLVM::getWithNewBitWidth(Phi->getType(), PromotedPredicateWidth),
        Phi->getNumIncomingValues(), Phi->getName() + ".promoted");
    for (unsigned i = 0; i < Phi->getNumIncomingValues(); ++i) {
      auto IncomingValue = Phi->getIncomingValue(i);
      auto IncomingBlock = Phi->getIncomingBlock(i);
      WidenedPhi->addIncoming(
          getExtendedValue(IncomingValue, IncomingBlock->getTerminator()),
          IncomingBlock);
    }
    return getTruncatedValue(WidenedPhi, Phi->getParent()->getFirstNonPHI());
  }
  // Process binary operators.
  IGC_ASSERT(isa<BinaryOperator>(Inst));
  Value *Op1 = getExtendedValue(Inst->getOperand(0), Inst),
        *Op2 = getExtendedValue(Inst->getOperand(1), Inst);
  Value *PromotedInst =
      IRB.CreateBinOp(cast<BinaryOperator>(Inst)->getOpcode(), Op1, Op2,
                      Inst->getName() + ".promoted");
  return getTruncatedValue(PromotedInst, Inst);
}

// Cleanup trunc->sext chains and lower trunc if there are remaining uses:
// trunc <n x i16> %val to <n x i1> => icmp %val, 0
// This is done in assumption that all bits in truncated value are the same
// (this is always true in predicate web).
static void foldTruncAndSExt(TruncInst *TI) {
  Value *SrcVal = TI->getOperand(0);
  Type *SrcTy = TI->getSrcTy();
  SmallVector<Instruction *, 4> ToErase;
  for (auto U : TI->users()) {
    auto *SI = dyn_cast<SExtInst>(U);
    if (!SI || SI->getDestTy() != SrcTy)
      continue;
    ToErase.push_back(SI);
  }
  for (auto SI : ToErase) {
    SI->replaceAllUsesWith(SrcVal);
    SI->eraseFromParent();
  }
  if (!TI->user_empty()) {
    auto Cmp =
        IRBuilder<>(TI).CreateICmpNE(SrcVal, Constant::getNullValue(SrcTy));
    TI->replaceAllUsesWith(Cmp);
  }
  TI->eraseFromParent();
}

class PredicateWeb {
public:
  template <class InputIt>
  PredicateWeb(InputIt first, InputIt last) : Web(first, last) {}
  void print(llvm::raw_ostream &O) const {
    for (auto Inst : Web)
      O << *Inst << '\n';
  }
  void dump() const { print(dbgs()); }
  bool isBeneficialToPromote() const {
    unsigned NumBinaryOps =
        std::count_if(Web.begin(), Web.end(),
                      [](auto *Inst) { return isa<BinaryOperator>(Inst); });
    return NumBinaryOps >= LogicOpsThreshold;
  }
  void doPromotion() const {
    // Do promotion.
    SmallVector<TruncInst *, 8> Worklist;
    for (auto Inst : Web) {
      auto PromotedInst = promoteInst(Inst);
      if (auto TI = dyn_cast<TruncInst>(PromotedInst))
        Worklist.push_back(TI);
      Inst->replaceAllUsesWith(PromotedInst);
      Inst->eraseFromParent();
    }
    // Do cleanup.
    for (auto TI : Worklist)
      foldTruncAndSExt(TI);
  }

private:
  std::set<Instruction *> Web;
};

bool GenXPromotePredicate::runOnFunction(Function &F) {
  // Put every predicate instruction into its own equivalence class.
  llvm::EquivalenceClasses<Instruction *> PredicateWebs;
  for (auto &I : instructions(F)) {
    if (!genx::isPredicate(&I))
      continue;
    if (!I.isBitwiseLogicOp() && !isa<PHINode>(&I))
      continue;
    PredicateWebs.insert(&I);
  }
  // Connect data-flow related instructions together.
  for (auto &EC : PredicateWebs) {
    Instruction *Inst = EC.getData();
    for (auto &Op : Inst->operands()) {
      Instruction *In = dyn_cast<Instruction>(Op);
      if (!In || PredicateWebs.findValue(In) == PredicateWebs.end())
        continue;
      PredicateWebs.unionSets(Inst, In);
    }
  }
  // Promote web if it is big enough (likely to cause flag spills).
  bool Modified = false;
  for (auto I = PredicateWebs.begin(), E = PredicateWebs.end(); I != E; ++I) {
    if (!I->isLeader())
      continue;
    PredicateWeb Web(PredicateWebs.member_begin(I), PredicateWebs.member_end());
    LLVM_DEBUG(dbgs() << "Predicate web:\n"; Web.dump());
    ++NumCollectedPredicateWebs;
    if (!Web.isBeneficialToPromote())
      continue;
    LLVM_DEBUG(dbgs() << "Beneficial to promote\n");
    Web.doPromotion();
    ++NumPromotedPredicateWebs;
    Modified = true;
  }
  return Modified;
}
