/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/PredicatedSplitter.hpp"
#include "Compiler/CISACodeGen/SoAAcrossFNUtil.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/ADT/MapVector.h>
#include <llvm/IR/Instructions.h>
#include "common/LLVMWarningsPop.hpp"

#include "llvmWrapper/IR/Instructions.h"
#include "Probe/Assertion.h"

using namespace llvm;
using namespace IGC;
using SoAAcrossFNUtil::LoadVia;

Value *PredicatedSplitter::getBase(unsigned I) const {
  Value *Incoming = Phi->getIncomingValue(I);
  Value *Base = SoAAcrossFNUtil::getRootBase(Incoming, DL, ExtraRoots);
  return Base ? Base : Incoming;
}

bool PredicatedSplitter::canSplit() const {
  for (unsigned I = 0; I < Phi->getNumIncomingValues(); ++I) {
    if (!canRematerialize(getBase(I)))
      return false;
  }
  return true;
}

void PredicatedSplitter::split(ArrayRef<LoadVia> Loads) {
  const unsigned N = Phi->getNumIncomingValues();

  MapVector<Value *, SmallVector<unsigned, 4>> BaseToIncoming;
  for (unsigned I = 0; I < N; ++I) {
    auto *Base = getBase(I);
    BaseToIncoming[Base].push_back(I);
  }

  // Handle the trivial case where predicated loads are not needed
  if (BaseToIncoming.size() == 1) {
    for (const LoadVia &LV : Loads) {
      Instruction *Load = LV.Load;
      IRBuilder<> B(Load);
      B.SetCurrentDebugLocation(Load->getDebugLoc());
      auto &[Base, SharedBase] = *BaseToIncoming.begin();
      Value *Ptr = SoAAcrossFNUtil::replayAddressChain(B, rematerialize(B, Base, SharedBase), LV.Chain, DL, ExtraRoots);
      Instruction *NewLoad;
      if (isa<PredicatedLoadIntrinsic>(Load)) {
        NewLoad = IGC::SoAAcrossFNUtil::createPredicatedLoad(B, Load, Ptr, B.getTrue());
      } else {
        NewLoad = Load->clone();
        NewLoad->setOperand(LoadInst::getPointerOperandIndex(), Ptr);
        B.Insert(NewLoad);
      }
      Load->replaceAllUsesWith(NewLoad);
    }
    return;
  }

  SmallVector<Value *, 4> IsSide;
  for (auto &[Base, SharedBase] : BaseToIncoming) {
    PHINode *P = PHINode::Create(Type::getInt1Ty(Phi->getContext()), N, Phi->getName() + ".isSide",
                                 IGCLLVM::insertPosition(Phi));
    for (unsigned J = 0; J < N; ++J) {
      P->addIncoming(ConstantInt::getBool(Phi->getContext(), llvm::is_contained(SharedBase, J)),
                     Phi->getIncomingBlock(J));
    }
    IsSide.push_back(P);
  }

  for (const LoadVia &LV : Loads) {
    Instruction *Load = LV.Load;
    IRBuilder<> B(Load);
    B.SetCurrentDebugLocation(Load->getDebugLoc());
    SmallVector<Value *, 4> Vals;
    for (auto Item : llvm::enumerate(BaseToIncoming)) {
      auto &[Base, SharedBase] = Item.value();
      Value *Ptr = SoAAcrossFNUtil::replayAddressChain(B, rematerialize(B, Base, SharedBase), LV.Chain, DL, ExtraRoots);
      Vals.push_back(SoAAcrossFNUtil::createPredicatedLoad(B, Load, Ptr, IsSide[Item.index()]));
    }
    Value *V = Vals.back();
    for (unsigned I = BaseToIncoming.size() - 1; I-- > 0;)
      V = B.CreateSelect(IsSide[I], Vals[I], V);
    Load->replaceAllUsesWith(V);
  }
}

bool PredicatedSplitter::canRematerialize(Value *V) const {
  auto *I = dyn_cast<Instruction>(V);
  // An incoming instruction has the value it had on the incoming edge at an access only if it is not redefined in
  // between, i.e. if it dominates the merge PHI. With a PHI as the user, dominates() holds only if I's block strictly
  // dominates the merge block, so instructions of the merge block itself (its PHIs, or a self-loop back-edge value)
  // are excluded.
  if (!I || DT.dominates(I, Phi))
    return true;
  if (!V->getType()->isPointerTy())
    return true; // carried by a PHI
  if (auto *GEP = dyn_cast<GetElementPtrInst>(I)) {
    return llvm::all_of(GEP->operands(), [&](Value *Op) { return canRematerialize(Op); });
  }
  if (isa<BitCastInst, AddrSpaceCastInst>(I))
    return canRematerialize(I->getOperand(0));
  return false;
}

Value *PredicatedSplitter::rematerialize(IRBuilder<> &B, Value *V, ArrayRef<unsigned> Incoming) {
  auto *I = dyn_cast<Instruction>(V);
  if (!I || DT.dominates(I, Phi))
    return V;
  if (!V->getType()->isPointerTy()) {
    Value *&P = Carried[{V, Incoming[0]}];
    if (!P) {
      PHINode *NewPhi = PHINode::Create(V->getType(), Phi->getNumIncomingValues(), V->getName() + ".carried",
                                        IGCLLVM::insertPosition(Phi));
      for (unsigned J = 0; J < Phi->getNumIncomingValues(); ++J)
        NewPhi->addIncoming(llvm::is_contained(Incoming, J) ? V : Constant::getNullValue(V->getType()),
                            Phi->getIncomingBlock(J));
      P = NewPhi;
    }
    return P;
  }
  IGC_ASSERT(isa<GetElementPtrInst>(I) || isa<BitCastInst>(I) || isa<AddrSpaceCastInst>(I));
  Instruction *New = I->clone();
  for (Use &Op : New->operands())
    Op.set(rematerialize(B, Op.get(), Incoming));
  return B.Insert(New, I->getName() + ".remat");
}
