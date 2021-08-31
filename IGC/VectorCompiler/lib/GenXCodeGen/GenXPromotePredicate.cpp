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

#include "GenX.h"
#include "GenXModule.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"
#include "Probe/Assertion.h"

#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;
using namespace genx;

static cl::opt<unsigned>
LogicOpsThreshold("logical-ops-threshold", cl::init(2), cl::Hidden,
                  cl::desc("Number of logical operations"));

namespace {

class GenXPromotePredicate : public FunctionPass {
public:
  static char ID;
  GenXPromotePredicate() : FunctionPass(ID) {}
  bool runOnFunction(Function &F) override;
  StringRef getPassName() const override { return "GenXPromotePredicate"; }
  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addPreserved<GenXModule>();
    AU.setPreservesCFG();
  }

private:
  bool matchOpnds(llvm::BasicBlock *UseBB, Value *V, unsigned &NumLogicOps);
  Value *rewriteTree(Instruction *Inst);
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

// This matches a common pattern like
//
// v1.merge(v2, (v3 > 0) | (v4 < 9))
//
// Or operation will be beformed on <n x i1> which may cause flag spills when n
// is large. We promote such computations into <n x i16>.
//
bool GenXPromotePredicate::runOnFunction(Function &F) {
  // Collect candidates.
  SmallVector<Instruction *, 8> Candidates;
  for (auto &BB : F.getBasicBlockList()) {
    for (auto &Inst : BB.getInstList()) {
      auto SI = dyn_cast<SelectInst>(&Inst);
      if (SI == nullptr || SI->use_empty())
        continue;

      // Match conditions with at least 32 elements.
      auto Cond = dyn_cast<Instruction>(SI->getCondition());
      if (!Cond || !Cond->getType()->isVectorTy())
        continue;
      if (cast<IGCLLVM::FixedVectorType>(Cond->getType())->getNumElements() <
          32)
        continue;

      // TODO: analyze when it is benefial to promote.
      unsigned NumLogicOps = 0;
      if (matchOpnds(SI->getParent(), Cond, NumLogicOps) &&
          NumLogicOps >= LogicOpsThreshold)
        Candidates.push_back(Cond);
    }
  }

  // Do promotions. This is a tree rewrite, with candidates as root,
  // comparisions or constants as leaf nodes.
  for (auto Inst : Candidates) {
    IGC_ASSERT(Inst->hasOneUse());
    Instruction *UI = Inst->user_back();
    Value *V = rewriteTree(Inst);
    IGC_ASSERT(isa<Instruction>(V));
    auto TI = TruncInst::Create(CastInst::Trunc, V, Inst->getType());
    TI->insertAfter(cast<Instruction>(V));
    TI->setDebugLoc(Inst->getDebugLoc());
    UI->replaceUsesOfWith(Inst, TI);
    RecursivelyDeleteTriviallyDeadInstructions(Inst);
  }

  return !Candidates.empty();
}

bool GenXPromotePredicate::matchOpnds(llvm::BasicBlock *UseBB, Value *V,
                                      unsigned &NumLogicOps) {
  auto Inst = dyn_cast<Instruction>(V);
  // Constants are OK.
  if (Inst == nullptr)
    return isa<Constant>(V);

  unsigned Opc = Inst->getOpcode();
  switch (Opc) {
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor:
    ++NumLogicOps;
    // Match local definitions only.
    if (!Inst->hasOneUse() || Inst->getParent() != UseBB)
      return false;

    // Recurse on its operands.
    return matchOpnds(UseBB, Inst->getOperand(0), NumLogicOps) &&
           matchOpnds(UseBB, Inst->getOperand(1), NumLogicOps);
  case Instruction::ICmp:
  case Instruction::FCmp:
    // Matching stops at local comparison operands.
    return Inst->hasOneUse() && Inst->getParent() == UseBB;
  default:
    break;
  }

  // Not a match.
  return false;
}
Value *GenXPromotePredicate::rewriteTree(Instruction *Inst) {
  IRBuilder<> Builder(Inst);
  unsigned N =
      cast<IGCLLVM::FixedVectorType>(Inst->getType())->getNumElements();
  VectorType *VT = IGCLLVM::FixedVectorType::get(Builder.getInt16Ty(), N);
  unsigned Opc = Inst->getOpcode();
  switch (Opc) {
  case Instruction::And:
  case Instruction::Or:
  case Instruction::Xor: {
    Value *Ops[] = {nullptr, nullptr};
    for (unsigned i : {0, 1}) {
      Value *Op = Inst->getOperand(i);
      if (auto C = dyn_cast<Constant>(Op))
        Ops[i] = Builder.CreateSExt(C, VT, ".sext");
      else if (auto I = dyn_cast<Instruction>(Op))
        Ops[i] = rewriteTree(I);
      else
        IGC_ASSERT_EXIT_MESSAGE(0, "out of sync");
    }

    Value *V = Builder.CreateBinOp(Instruction::BinaryOps(Opc), Ops[0], Ops[1]);
    V->takeName(Inst);
    if (auto I = dyn_cast<Instruction>(V))
      I->setDebugLoc(Inst->getDebugLoc());
    return V;
  }
  case Instruction::ICmp:
  case Instruction::FCmp: {
    auto V = Builder.CreateSExt(Inst, VT, ".sext");
    if (auto I = dyn_cast<Instruction>(V)) {
      I->setDebugLoc(Inst->getDebugLoc());
      Inst->moveBefore(I);
    }
    return V;
  }
  default:
    break;
  }

  IGC_ASSERT_EXIT_MESSAGE(0, "out of sync");
}
