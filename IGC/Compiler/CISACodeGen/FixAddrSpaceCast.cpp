/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#define DEBUG_TYPE "addrspacecast-fixer"
#include "Compiler/CISACodeGen/FixAddrSpaceCast.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/IGCPassSupport.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/IR/BasicBlock.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace {
class AddrSpaceCastFixing : public FunctionPass {
  const unsigned GAS = ADDRESS_SPACE_GENERIC;
  const unsigned PrivateAS = ADDRESS_SPACE_PRIVATE;

public:
  static char ID;

  AddrSpaceCastFixing() : FunctionPass(ID) { initializeAddrSpaceCastFixingPass(*PassRegistry::getPassRegistry()); }

  bool runOnFunction(Function &) override;

  void getAnalysisUsage(AnalysisUsage &AU) const override { AU.setPreservesCFG(); }

private:
  bool fixOnBasicBlock(BasicBlock *) const;

  bool fixCase1(Instruction *I, BasicBlock::iterator &BI) const;
  bool fixCase2(Instruction *I, BasicBlock::iterator &BI) const;
};
} // End anonymous namespace

FunctionPass *IGC::createFixAddrSpaceCastPass() { return new AddrSpaceCastFixing(); }

char AddrSpaceCastFixing::ID = 0;

#define PASS_FLAG "igc-addrspacecast-fix"
#define PASS_DESC "Fix invalid addrspacecast-relevant patterns"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
namespace IGC {
IGC_INITIALIZE_PASS_BEGIN(AddrSpaceCastFixing, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(AddrSpaceCastFixing, PASS_FLAG, PASS_DESC, PASS_CFG_ONLY, PASS_ANALYSIS)
} // namespace IGC

static bool hasPHIDifferentAddrSpacesOperands(PHINode *PHI) {
  SmallSet<unsigned, 4> AddrSpaces;
  for (auto &Incoming : PHI->incoming_values()) {
    if (auto *AddrSpaceCast = dyn_cast<AddrSpaceCastInst>(Incoming)) {
      AddrSpaces.insert(AddrSpaceCast->getSrcAddressSpace());
    } else if (auto *GetElementPtr = dyn_cast<GetElementPtrInst>(Incoming)) {
      AddrSpaces.insert(GetElementPtr->getPointerAddressSpace());
    } else {
      // Unexpected operand type.
      return false;
    }
  }
  // Limit to only those PHIs that have LOCAL and other address spaces.
  return AddrSpaces.size() > 1 && AddrSpaces.contains(IGC::ADDRESS_SPACE_LOCAL);
}

static bool isPhiUseLowerable(PHINode *PHI) {
  if (!PHI->hasOneUse()) {
    return false;
  }
  if (isa<LoadInst>(*(PHI->user_begin()))) {
    return true;
  }
  return isa<BitCastInst>(*(PHI->user_begin())) && PHI->user_begin()->hasOneUse() &&
         isa<LoadInst>(*(PHI->user_begin()->user_begin()));
}

static void lowerLoadsfromPHI(PHINode *PHI) {
  SmallVector<Instruction *, 8> ToErase;
  SmallVector<Instruction *, 2> InsertChain;

  LoadInst *PHILoad = nullptr;
  if (isa<LoadInst>(*(PHI->user_begin()))) {
    InsertChain.push_back(cast<Instruction>(*PHI->user_begin()));
    PHILoad = cast<LoadInst>(*PHI->user_begin());
  } else {
    InsertChain.push_back(cast<Instruction>(*PHI->user_begin()));
    InsertChain.push_back(cast<Instruction>(*PHI->user_begin()->user_begin()));
    PHILoad = cast<LoadInst>(*PHI->user_begin()->user_begin());
  }
  IGC_ASSERT_MESSAGE(PHILoad != nullptr, "Expecting a load instruction");
  llvm::append_range(ToErase, llvm::reverse(InsertChain));
  ToErase.push_back(cast<Instruction>(PHI));

  SmallMapVector<Instruction *, BasicBlock *, 8> NewLoads;
  for (auto &Incoming : PHI->incoming_values()) {
    auto *LoadSource = cast<Instruction>(Incoming);
    auto *AddrSpaceCast = dyn_cast<AddrSpaceCastInst>(Incoming);

    for (auto *I : InsertChain) {

      if (AddrSpaceCast) {
        IRBuilder<> Builder(LoadSource);
        if (isa<BitCastInst>(I)) {
          auto *NewBitcast = Builder.CreateBitCast(
              AddrSpaceCast->getOperand(0), PointerType::get(PHILoad->getType(), AddrSpaceCast->getSrcAddressSpace()));
          LoadSource = cast<Instruction>(NewBitcast);
        } else if (isa<LoadInst>(I)) {
          auto *NewLoad = Builder.CreateLoad(PHILoad->getType(), AddrSpaceCast->getOperand(0));
          NewLoad->copyMetadata(*I);
          LoadSource = NewLoad;
          NewLoads.insert({NewLoad, PHI->getIncomingBlock(Incoming)});
        } else {
          IGC_ASSERT_MESSAGE(false, "Unexpected instruction in the use chain.");
        }
        if (AddrSpaceCast->hasOneUse()) {
          ToErase.push_back(AddrSpaceCast);
        }
        AddrSpaceCast = nullptr;
        continue;
      }

      Instruction *NewInst = I->clone();
      NewInst->setOperand(0, LoadSource);
      NewInst->insertAfter(LoadSource);
      LoadSource = NewInst;
      if (isa<LoadInst>(NewInst)) {
        NewLoads.insert({NewInst, PHI->getIncomingBlock(Incoming)});
      }
    }
  }
  IRBuilder<> Builder(PHI);
  auto *NewPHI = Builder.CreatePHI(PHILoad->getType(), NewLoads.size());

  llvm::for_each(NewLoads, [&](const auto &Pair) { NewPHI->addIncoming(Pair.first, Pair.second); });
  PHILoad->replaceAllUsesWith(NewPHI);

  for_each(ToErase, [](Instruction *I) { I->eraseFromParent(); });
}

static bool removeAddrSpaceCastsFromPhiIncomingBBs(Function &F) {
  bool Changed = false;

  SmallVector<PHINode *, 8> PhisToModify;
  for (auto &I : instructions(F)) {
    if (auto *PHI = dyn_cast<PHINode>(&I)) {
      if (hasPHIDifferentAddrSpacesOperands(PHI) && isPhiUseLowerable(PHI)) {
        PhisToModify.push_back(PHI);
      }
    }
  }
  for (auto *PHI : PhisToModify) {
    lowerLoadsfromPHI(PHI);
    Changed = true;
  }
  return Changed;
}

bool AddrSpaceCastFixing::runOnFunction(Function &F) {
  bool Changed = false;
  for (auto &BB : F) {
    Changed |= fixOnBasicBlock(&BB);
  }
  if (IGC_IS_FLAG_ENABLED(AddressSpacePhiPropagation)) {
    Changed |= removeAddrSpaceCastsFromPhiIncomingBBs(F);
  }
  return Changed;
}

bool AddrSpaceCastFixing::fixOnBasicBlock(BasicBlock *BB) const {
  bool Changed = false;

  for (auto BI = BB->begin(), BE = BB->end(); BI != BE; /* EMPTY */) {
    Instruction *I = &(*BI++);
    if (fixCase1(I, BI)) {
      Changed = true;
      continue;
    }
    if (fixCase2(I, BI)) {
      Changed = true;
      continue;
    }
  }

  return Changed;
}

/// fixCase1 - Convert pair of `ptrtoint`/`inttoptr` through `i64` back to
/// `addrspacecast`.
bool AddrSpaceCastFixing::fixCase1(Instruction *I, BasicBlock::iterator &BI) const {
  // Find the eligible pair of `ptrtoint`/`inttoptr` and convert them back
  // to `addrspacecast`.
  IntToPtrInst *I2P = dyn_cast<IntToPtrInst>(I);
  if (!I2P || !I2P->getType()->isPointerTy())
    return false;
  PointerType *DstPtrTy = cast<PointerType>(I2P->getType());
  PtrToIntInst *P2I = dyn_cast<PtrToIntInst>(I2P->getOperand(0));
  if (!P2I)
    return false;
  if (!P2I->hasOneUse() || !P2I->getType()->isIntegerTy(64))
    return false;

  Value *V = P2I->getOperand(0);
  PointerType *SrcPtrTy = cast<PointerType>(V->getType());

  // Skip generating `bitcast` if their element types are different. Leave that
  // canonicalization in GAS resolver.

  // Create `addrspacecast` if necessary.
  if (SrcPtrTy->getAddressSpace() != DstPtrTy->getAddressSpace())
    V = CastInst::Create(Instruction::AddrSpaceCast, V, DstPtrTy, I->getName() + ".fix1.addrspacecast", I);
  else
    V = CastInst::Create(Instruction::BitCast, V, DstPtrTy, I->getName() + ".fix1.bitcast", I);

  // Remove the short sequence of `ptrtoint` followed by `inttoptr`.
  if (Instruction *VInst = dyn_cast<Instruction>(V)) {
    VInst->setDebugLoc(I2P->getDebugLoc());
  }
  I2P->replaceAllUsesWith(V);
  I2P->eraseFromParent();
  P2I->eraseFromParent();

  if (isa<Instruction>(V))
    BI = BasicBlock::iterator(cast<Instruction>(V));

  return true;
}

/// fixCase2 - Convert the following sequence into a valid one.
///
/// (addrspacecast-gas
///   (select %cond (addrspacecast-private %ptr1)
///                 (addrspacecast-private %ptr2)))
///
/// into
///
/// (select %cond (addrspacecast-gas %ptr1)
///               (addrspacecast-gas %ptr2))
///
bool AddrSpaceCastFixing::fixCase2(Instruction *I, BasicBlock::iterator &BI) const {
  AddrSpaceCastInst *CI = dyn_cast<AddrSpaceCastInst>(I);
  // Skip if it's not an addrspacecast to GAS
  if (!CI || cast<PointerType>(CI->getType())->getAddressSpace() != GAS)
    return false;

  // Skip if it's not a select of private pointers.
  SelectInst *SI = dyn_cast<SelectInst>(CI->getOperand(0));
  if (!SI || cast<PointerType>(SI->getType())->getAddressSpace() != PrivateAS)
    return false;

  PointerType *DstPtrTy = cast<PointerType>(CI->getType());

  // Skip if T/F values don't agree on address space.
  Value *TVal = SI->getTrueValue();
  if (TVal->hasOneUse() && isa<AddrSpaceCastInst>(TVal))
    TVal->mutateType(DstPtrTy);
  else
    TVal = CastInst::Create(Instruction::AddrSpaceCast, TVal, DstPtrTy, TVal->getName() + ".fix2.addrspacecast", SI);

  Value *FVal = SI->getFalseValue();
  if (FVal->hasOneUse() && isa<AddrSpaceCastInst>(FVal))
    FVal->mutateType(DstPtrTy);
  else
    FVal = CastInst::Create(Instruction::AddrSpaceCast, FVal, DstPtrTy, FVal->getName() + ".fix2.addrspacecast", SI);

  SI->setOperand(1, TVal);
  SI->setOperand(2, FVal);
  SI->mutateType(DstPtrTy);

  CI->replaceAllUsesWith(SI);
  CI->eraseFromParent();

  return true;
}
