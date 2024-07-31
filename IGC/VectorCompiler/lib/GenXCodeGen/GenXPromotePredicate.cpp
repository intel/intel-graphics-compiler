/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2024 Intel Corporation

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

#include "GenX.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "llvm/ADT/EquivalenceClasses.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/CodeGen/TargetPassConfig.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/InitializePasses.h"
#include "llvm/Pass.h"

#include "llvmWrapper/IR/DerivedTypes.h"

#define DEBUG_TYPE "genx-promote-predicate"

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
    AU.addRequired<TargetPassConfig>();
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
INITIALIZE_PASS_DEPENDENCY(TargetPassConfig)
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

static Value *scalarizeInputPredicate(Value *V, Type *Ty, IRBuilder<> &IRB) {
  if (auto *C = dyn_cast<Constant>(V)) {
    if (C->isAllOnesValue())
      return Constant::getAllOnesValue(Ty);
    if (C->isNullValue())
      return Constant::getNullValue(Ty);
  }
  return IRB.CreateBitCast(V, Ty);
}

// Promote a vector predicate operation into scalar integer one.
static Value *promoteInstToScalar(Instruction *Inst) {
  IRBuilder<> IRB(Inst);

  auto *VTy = cast<IGCLLVM::FixedVectorType>(Inst->getType());
  auto Width = VTy->getNumElements();
  IGC_ASSERT(VTy->isIntOrIntVectorTy(1));
  IGC_ASSERT(Width == 8 || Width == 16 || Width == 32);

  auto *STy = IRB.getIntNTy(Width);

  // Special case - phi node.
  if (auto *Phi = dyn_cast<PHINode>(Inst)) {
    auto *ScalarPhi = IRB.CreatePHI(STy, Phi->getNumIncomingValues(),
                                    Phi->getName() + ".scalar");
    for (unsigned I = 0; I < Phi->getNumIncomingValues(); ++I) {
      auto *IncomingBlock = Phi->getIncomingBlock(I);
      auto *IncomingValue = Phi->getIncomingValue(I);

      IRB.SetInsertPoint(IncomingBlock->getTerminator());
      auto *ScalarValue = scalarizeInputPredicate(IncomingValue, STy, IRB);

      ScalarPhi->addIncoming(ScalarValue, IncomingBlock);
    }

    IRB.SetInsertPoint(Phi->getParent()->getFirstNonPHI());
    return IRB.CreateBitCast(ScalarPhi, VTy);
  }

  IGC_ASSERT(isa<BinaryOperator>(Inst));

  auto *Op1 = scalarizeInputPredicate(Inst->getOperand(0), STy, IRB);
  auto *Op2 = scalarizeInputPredicate(Inst->getOperand(1), STy, IRB);
  auto *ScalarInst = IRB.CreateBinOp(cast<BinaryOperator>(Inst)->getOpcode(),
                                     Op1, Op2, Inst->getName() + ".scalar");
  return IRB.CreateBitCast(ScalarInst, VTy);
}

// Promote one predicate instruction to grf - promote all its operands and
// instruction itself, and then sink the result back to predicate.
static Value *promoteInst(Instruction *Inst, bool AllowScalarPromotion) {
  if (auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Inst->getType());
      VTy && AllowScalarPromotion) {
    IGC_ASSERT(VTy->isIntOrIntVectorTy(1));
    auto Width = VTy->getNumElements();

    if (Width == 8 || Width == 16 || Width == 32)
      return promoteInstToScalar(Inst);
  }

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

static void foldBitcast(BitCastInst *Cast) {
  auto *Src = Cast->getOperand(0);
  auto *SrcTy = Src->getType();

  SmallVector<User *, 4> ToErase;
  llvm::copy_if(Cast->users(), std::back_inserter(ToErase), [SrcTy](auto *U) {
    return isa<BitCastInst>(U) && U->getType() == SrcTy;
  });

  for (auto *U : ToErase) {
    auto *I = cast<Instruction>(U);
    I->replaceAllUsesWith(Src);
    I->eraseFromParent();
  }

  if (Cast->user_empty())
    Cast->eraseFromParent();
}

class PredicateWeb {
public:
  template <class InputIt>
  PredicateWeb(InputIt First, InputIt Last, bool AllowScalarAllAny)
      : Web(First, Last), AllowScalarAllAny(AllowScalarAllAny) {}
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
    auto AllowScalar = true;
    if (!AllowScalarAllAny)
      AllowScalar = llvm::none_of(Web, [](auto *Inst) {
        return llvm::any_of(Inst->users(), [](auto *U) {
          auto IID = vc::getAnyIntrinsicID(U);
          return IID == GenXIntrinsic::genx_any ||
                 IID == GenXIntrinsic::genx_all;
        });
      });

    // Do promotion.
    SmallVector<Instruction *, 8> Worklist;
    for (auto *Inst : Web) {
      auto *PromotedInst = promoteInst(Inst, AllowScalar);

      if (isa<TruncInst>(PromotedInst) || isa<BitCastInst>(PromotedInst))
        Worklist.push_back(cast<Instruction>(PromotedInst));

      Inst->replaceAllUsesWith(PromotedInst);
      Inst->eraseFromParent();
    }
    // Do cleanup.
    for (auto *I : Worklist)
      if (auto *Trunc = dyn_cast<TruncInst>(I))
        foldTruncAndSExt(Trunc);
      else if (auto *Cast = dyn_cast<BitCastInst>(I))
        foldBitcast(Cast);
  }

private:
  SmallPtrSet<Instruction *, 16> Web;
  bool AllowScalarAllAny;
};

constexpr const char IdxMDName[] = "pred.index";
// Comparator to keep sequence of instructions - otherwise it will compare
// dynamically allocated pointers
struct Comparator {
  long getMd(Instruction *const &I) const {
    auto *IMd = I->getMetadata(IdxMDName);
    IGC_ASSERT_EXIT(IMd);
    return cast<ConstantInt>(
               cast<ConstantAsMetadata>(IMd->getOperand(0).get())->getValue())
        ->getZExtValue();
  }
  bool operator()(Instruction *const &Lhs, Instruction *const &Rhs) const {
    return getMd(Lhs) > getMd(Rhs);
  }
};

bool GenXPromotePredicate::runOnFunction(Function &F) {
  auto &ST = getAnalysis<TargetPassConfig>()
                 .getTM<GenXTargetMachine>()
                 .getGenXSubtarget();
  bool AllowScalarAllAny = !ST.hasFusedEU();

  // Put every predicate instruction into its own equivalence class.
  long Idx = 0;
  llvm::EquivalenceClasses<Instruction *, Comparator> PredicateWebs;
  for (auto &I : instructions(F)) {
    if (!genx::isPredicate(&I))
      continue;
    if (!I.isBitwiseLogicOp() && !isa<PHINode>(&I))
      continue;
    auto &Ctx = I.getContext();
    auto *MD = ConstantAsMetadata::get(
        ConstantInt::get(Ctx, llvm::APInt(64, ++Idx, false)));
    I.setMetadata(IdxMDName, MDNode::get(Ctx, MD));
    PredicateWebs.insert(&I);
  }
  // Connect data-flow related instructions together.
  for (auto &EC : PredicateWebs) {
    Instruction *Inst = EC.getData();
    for (auto &Op : Inst->operands()) {
      Instruction *In = dyn_cast<Instruction>(Op);

      if (!In || !In->hasMetadata(IdxMDName))
        continue;
      PredicateWebs.unionSets(Inst, In);
    }
  }
  // Promote web if it is big enough (likely to cause flag spills).
  bool Modified = false;
  for (auto I = PredicateWebs.begin(), E = PredicateWebs.end(); I != E; ++I) {
    if (!I->isLeader())
      continue;
    PredicateWeb Web(PredicateWebs.member_begin(I), PredicateWebs.member_end(),
                     AllowScalarAllAny);
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
