/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXDeadVectorRemoval
/// -----------------------
///
/// GenXDeadVectorRemoval - dead code removal pass that uses analysis of
/// individual elements of vector types
///
/// Pass can do the following modifications to the code:
///
/// 1. If any operand in the instruction turns out to be unused, it is
///    completely replaced with undef. If operand is constant and partially
//     unused, these unused elements are replaced with undefs to simplify work
//     for ConstantLoader in future
//  2. If whole value is not used it is removed from the code like in
//     traditional DCE
//===----------------------------------------------------------------------===//
#include "GenX.h"
#include "GenXLiveElements.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"

#include "Probe/Assertion.h"
#include "vc/Utils/General/IndexFlattener.h"

#define DEBUG_TYPE "GENX_DEAD_VECTOR_REMOVAL"

using namespace llvm;
using namespace genx;

STATISTIC(NumDeletedInsts, "Number of deleted instructions");
STATISTIC(NumSimplifiedUses, "Number of simplified uses");

namespace {

// GenXDeadVectorRemoval : dead vector element removal pass
class GenXDeadVectorRemoval : public FunctionPass {
public:
  static char ID;
  explicit GenXDeadVectorRemoval() : FunctionPass(ID) {}
  StringRef getPassName() const override {
    return "GenX dead vector element removal pass";
  }
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

private:
  Value *trySimplify(Value *V, const LiveElements &LiveElems);
  Constant *trySimplify(Constant *C, const LiveElements &LiveElems);
  Constant *trySimplify(ConstantAggregate *CA, const LiveElements &LiveElems);
  Constant *trySimplify(ConstantData *CD, const LiveElements &LiveElems);
  Constant *trySimplify(ConstantDataSequential *CDS,
                        const LiveElements &LiveElems);
};

} // end anonymous namespace

char GenXDeadVectorRemoval::ID = 0;

namespace llvm {
void initializeGenXDeadVectorRemovalPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXDeadVectorRemoval, "GenXDeadVectorRemoval",
                      "GenXDeadVectorRemoval", false, false)
INITIALIZE_PASS_DEPENDENCY(GenXFuncLiveElements)
INITIALIZE_PASS_END(GenXDeadVectorRemoval, "GenXDeadVectorRemoval",
                    "GenXDeadVectorRemoval", false, false)

FunctionPass *llvm::createGenXDeadVectorRemovalPass() {
  initializeGenXDeadVectorRemovalPass(*PassRegistry::getPassRegistry());
  return new GenXDeadVectorRemoval();
}

void GenXDeadVectorRemoval::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<GenXFuncLiveElements>();
  AU.setPreservesCFG();
}

/***********************************************************************
 * GenXDeadVectorRemoval::runOnFunction : process one function
 */
bool GenXDeadVectorRemoval::runOnFunction(Function &F) {
  auto &LE = getAnalysis<GenXFuncLiveElements>();
  bool Modified = false;
  SmallVector<Instruction *, 8> ToErase;
  for (auto &I : instructions(F)) {
    LLVM_DEBUG(dbgs() << "Processing instruction " << I << "\n");
    for (auto &U : I.operands()) {
      auto *OldV = cast<Value>(U);
      auto *NewV = trySimplify(OldV, LE.getLiveElements(&U));
      if (NewV != OldV) {
        LLVM_DEBUG(dbgs() << "Replacing " << *OldV << " with " << *NewV
                          << "\n");
        U.set(NewV);
        NumSimplifiedUses++;
        Modified = true;
      }
    }

    if (LE.getLiveElements(&I).isAllDead()) {
      ToErase.push_back(&I);
    } else if (GenXIntrinsic::isWrRegion(&I) ||
               vc::getAnyIntrinsicID(&I) == GenXIntrinsic::genx_wrpredregion) {
      auto NewValueOp =
          I.getOperand(GenXIntrinsic::GenXRegion::NewValueOperandNum);
      if (isa<UndefValue>(NewValueOp)) {
        LLVM_DEBUG(dbgs() << "Bypassing " << I << "\n");
        auto OldValueOp = I.getOperand(GenXIntrinsic::GenXRegion::OldValueOperandNum);
        I.replaceAllUsesWith(OldValueOp);
        ToErase.push_back(&I);
      }
    }
  }

  for (auto *Inst : ToErase) {
    IGC_ASSERT(Inst->use_empty());
    LLVM_DEBUG(dbgs() << "Deleting instruction " << *Inst << "\n");
    Inst->eraseFromParent();
    NumDeletedInsts++;
    Modified = true;
  }

  return Modified;
}

Value *GenXDeadVectorRemoval::trySimplify(Value *V,
                                          const LiveElements &LiveElems) {
  if (LiveElems.isAllDead())
    return UndefValue::get(V->getType());

  if (auto *C = dyn_cast<Constant>(V))
    return trySimplify(C, LiveElems);

  return V;
}

Constant *GenXDeadVectorRemoval::trySimplify(Constant *C,
                                             const LiveElements &LiveElems) {
  if (!LiveElems.isAnyDead())
    return C;

  // Do not modify predicates. GenXConstant gets no befefit from it
  if (C->getType()->getScalarType()->isIntegerTy(1))
    return C;

  if (auto CA = dyn_cast<ConstantAggregate>(C))
    return trySimplify(CA, LiveElems);

  if (auto CD = dyn_cast<ConstantData>(C))
    return trySimplify(CD, LiveElems);

  return C;
}

Constant *GenXDeadVectorRemoval::trySimplify(ConstantAggregate *CA,
                                             const LiveElements &LiveElems) {
  if (isa<ConstantVector>(CA)) {
    IGC_ASSERT(LiveElems.size() == 1);
    SmallVector<Constant *, 8> Elems;
    for (auto &U : CA->operands())
      Elems.push_back(LiveElems[0][U.getOperandNo()]
                          ? cast<Constant>(U.get())
                          : UndefValue::get(U->getType()));
    return ConstantVector::get(Elems);
  }

  if (isa<ConstantStruct>(CA)) {
    IGC_ASSERT(LiveElems.size() == CA->getNumOperands());
    SmallVector<Constant *, 8> Elems;
    for (auto &U : CA->operands()) {
      auto LE = LiveElems[U.getOperandNo()];
      auto OpLiveElems = LiveElements(std::move(LE));
      Elems.push_back(OpLiveElems.isAllDead()
                          ? UndefValue::get(U->getType())
                          : trySimplify(cast<Constant>(U.get()), OpLiveElems));
    }
    return ConstantStruct::get(cast<StructType>(CA->getType()), Elems);
  }

  return CA;
}

Constant *GenXDeadVectorRemoval::trySimplify(ConstantData *CD,
                                             const LiveElements &LiveElems) {
  if (isa<UndefValue>(CD) || isa<ConstantAggregateZero>(CD))
    return CD;

  IGC_ASSERT(LiveElems.size() == 1);
  if (auto CDS = dyn_cast<ConstantDataSequential>(CD))
    return trySimplify(CDS, LiveElems);

  IGC_ASSERT(LiveElems[0].size() == 1);
  return LiveElems[0][0] ? CD : UndefValue::get(CD->getType());
}

Constant *GenXDeadVectorRemoval::trySimplify(ConstantDataSequential *CDS,
                                             const LiveElements &LiveElems) {
  if (auto CDV = dyn_cast<ConstantDataVector>(CDS)) {
    SmallVector<Constant *, 8> Elems;
    for (unsigned Idx = 0; Idx < CDV->getNumElements(); Idx++) {
      auto Elem = CDV->getElementAsConstant(Idx);
      Elems.push_back(LiveElems[0][Idx] ? Elem
                                        : UndefValue::get(Elem->getType()));
    }
    return ConstantVector::get(Elems);
  }

  return CDS;
}
