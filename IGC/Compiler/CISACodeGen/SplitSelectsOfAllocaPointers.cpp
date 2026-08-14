/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SplitSelectsOfAllocaPointers.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallPtrSet.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/ModuleAllocaAnalysis.hpp"
#include "GenISAIntrinsics/GenIntrinsicInst.h"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/Analysis/ValueTracking.h"
#include "llvmWrapper/IR/Instructions.h"

#include "Probe/Assertion.h"

using namespace llvm;

namespace {

// Walk from a leaf load/store back to the select, collecting the bitcast/addrspacecast chain.
void collectCastChain(SelectInst *SI, Instruction *LeafUser, SmallVectorImpl<Instruction *> &Chain) {
  IGC_ASSERT(isa<LoadInst>(LeafUser) || isa<StoreInst>(LeafUser));
  Value *Cur = isa<LoadInst>(LeafUser) ? cast<LoadInst>(LeafUser)->getPointerOperand()
                                       : cast<StoreInst>(LeafUser)->getPointerOperand();
  while (Cur != SI) {
    IGC_ASSERT_MESSAGE(isa<BitCastInst>(Cur) || isa<AddrSpaceCastInst>(Cur),
                       "select pointer chain must consist of bitcast/addrspacecast only");
    auto *Cast = cast<CastInst>(Cur);
    Chain.push_back(Cast);
    Cur = Cast->getOperand(0);
  }
  std::reverse(Chain.begin(), Chain.end());
}

// Clone the cast chain at the current builder insertion point, starting from Root.
// Returns the final pointer value (head of chain, replicated).
Value *cloneCastChain(IRBuilder<> &B, Value *Root, ArrayRef<Instruction *> Chain) {
  Value *Cur = Root;
  for (Instruction *I : Chain) {
    if (auto *BC = dyn_cast<BitCastInst>(I))
      Cur = B.CreateBitCast(Cur, BC->getDestTy());
    else if (auto *ASC = dyn_cast<AddrSpaceCastInst>(I))
      Cur = B.CreateAddrSpaceCast(Cur, ASC->getDestTy());
  }
  return Cur;
}

Instruction *createPredicatedLoad(IRBuilder<> &B, LoadInst *Model, Value *Ptr, Value *Pred) {
  Module *Mod = B.GetInsertBlock()->getModule();
  Type *MemType = Model->getType();
  Type *ITys[3] = {MemType, Ptr->getType(), MemType};
  Function *PredLoadFunc = GenISAIntrinsic::getDeclaration(Mod, GenISAIntrinsic::GenISA_PredicatedLoad, ITys);
  Value *AlignV = ConstantInt::get(Type::getInt64Ty(B.getContext()), IGCLLVM::getAlignmentValue(Model));
  Value *MergeV = PoisonValue::get(MemType);
  Value *Args[4] = {Ptr, AlignV, Pred, MergeV};
  Instruction *PredLoad = B.CreateCall(PredLoadFunc, Args);
  PredLoad->setDebugLoc(Model->getDebugLoc());
  return PredLoad;
}

// Transform: %v = load (chain(SI))  ==> %va = load chain(A); %vb = load chain(B); %v = select c, va, vb
void splitLoadOfSelectPtr(LoadInst *LI, SelectInst *SI, bool UsePredicatedLoads) {
  SmallVector<Instruction *, 4> Chain;
  collectCastChain(SI, LI, Chain);
  IRBuilder<> B(LI);
  B.SetCurrentDebugLocation(LI->getDebugLoc());
  Value *Cond = SI->getCondition();
  Value *PtrTrue = cloneCastChain(B, SI->getTrueValue(), Chain);
  Value *PtrFalse = cloneCastChain(B, SI->getFalseValue(), Chain);
  Value *VTrue = nullptr;
  Value *VFalse = nullptr;
  if (UsePredicatedLoads) {
    Value *NotCond = B.CreateNot(Cond);
    VTrue = createPredicatedLoad(B, LI, PtrTrue, Cond);
    VFalse = createPredicatedLoad(B, LI, PtrFalse, NotCond);
  } else {
    auto *LTrue = cast<LoadInst>(LI->clone());
    LTrue->setOperand(LI->getPointerOperandIndex(), PtrTrue);
    B.Insert(LTrue);
    auto *LFalse = cast<LoadInst>(LI->clone());
    LFalse->setOperand(LI->getPointerOperandIndex(), PtrFalse);
    B.Insert(LFalse);
    VTrue = LTrue;
    VFalse = LFalse;
  }
  Value *V = B.CreateSelect(Cond, VTrue, VFalse);
  LI->replaceAllUsesWith(V);
  LI->eraseFromParent();
}

// Transform: store v, chain(SI)  ==> if (cond) store v, chain(A); else store v, chain(B);
void splitStoreOfSelectPtr(StoreInst *StI, SelectInst *SI) {
  SmallVector<Instruction *, 4> Chain;
  collectCastChain(SI, StI, Chain);
  Instruction *ThenTerm = nullptr;
  Instruction *ElseTerm = nullptr;
  SplitBlockAndInsertIfThenElse(SI->getCondition(), StI, &ThenTerm, &ElseTerm);
  {
    IRBuilder<> B(ThenTerm);
    B.SetCurrentDebugLocation(StI->getDebugLoc());
    Value *PtrTrue = cloneCastChain(B, SI->getTrueValue(), Chain);
    auto *STrue = cast<StoreInst>(StI->clone());
    STrue->setOperand(StI->getPointerOperandIndex(), PtrTrue);
    B.Insert(STrue);
  }
  {
    IRBuilder<> B(ElseTerm);
    B.SetCurrentDebugLocation(StI->getDebugLoc());
    Value *PtrFalse = cloneCastChain(B, SI->getFalseValue(), Chain);
    auto *SFalse = cast<StoreInst>(StI->clone());
    SFalse->setOperand(StI->getPointerOperandIndex(), PtrFalse);
    B.Insert(SFalse);
  }
  StI->eraseFromParent();
}

// Walk from SI through bitcast/addrspacecast users, collecting the leaf
// load/store consumers. Returns false if any non-supported user is encountered
// (e.g. another SELECT, a GEP, a PHI), in which case the SI is not safe to split.
bool collectSelectLeafConsumers(SelectInst *SI, SmallVectorImpl<Instruction *> &Loads,
                                SmallVectorImpl<Instruction *> &Stores, SmallVectorImpl<Instruction *> &CastsToErase) {
  SmallVector<Instruction *, 8> Stack;
  SmallPtrSet<Instruction *, 16> Visited;
  Stack.push_back(SI);
  while (!Stack.empty()) {
    Instruction *Cur = Stack.pop_back_val();
    if (!Visited.insert(Cur).second) {
      continue;
    }
    for (User *U : Cur->users()) {
      if (auto *LI = dyn_cast<LoadInst>(U)) {
        if (!LI->isSimple()) {
          return false;
        }
        Loads.push_back(LI);
      } else if (auto *St = dyn_cast<StoreInst>(U)) {
        // Only a store *through* the merged pointer can be split. If the value
        // being stored is the merged pointer too, cloning the store would leave
        // the select live in the clones' value operand, so bail out.
        if (St->getPointerOperand() != Cur || St->getValueOperand() == Cur || !St->isSimple()) {
          return false;
        }
        Stores.push_back(St);
      } else if (isa<BitCastInst>(U) || isa<AddrSpaceCastInst>(U)) {
        CastsToErase.push_back(cast<Instruction>(U));
        Stack.push_back(cast<Instruction>(U));
      } else {
        return false;
      }
    }
  }
  return true;
}

bool splitSelectsOfAllocaPointers(Function &F, bool UsePredicatedLoads) {
  SmallVector<SelectInst *, 16> WorkList;
  const DataLayout &DL = F.getParent()->getDataLayout();
  for (Instruction &I : instructions(F)) {
    auto *SI = dyn_cast<SelectInst>(&I);
    if (!SI || !SI->getType()->isPointerTy()) {
      continue;
    }
    if (isa<AllocaInst>(IGCLLVM::getUnderlyingObject(SI->getTrueValue(), DL)) ||
        isa<AllocaInst>(IGCLLVM::getUnderlyingObject(SI->getFalseValue(), DL))) {
      WorkList.push_back(SI);
    }
  }
  bool Changed = false;
  SmallVector<Instruction *, 8> Loads;
  SmallVector<Instruction *, 4> Stores;
  SmallVector<Instruction *, 8> CastsToErase;
  for (SelectInst *SI : WorkList) {
    Loads.clear();
    Stores.clear();
    CastsToErase.clear();
    if (!collectSelectLeafConsumers(SI, Loads, Stores, CastsToErase)) {
      continue;
    }
    if (Loads.empty() && Stores.empty()) {
      continue;
    }
    for (Instruction *LI : Loads) {
      splitLoadOfSelectPtr(cast<LoadInst>(LI), SI, UsePredicatedLoads);
    }
    for (Instruction *St : Stores) {
      splitStoreOfSelectPtr(cast<StoreInst>(St), SI);
    }
    llvm::for_each(llvm::reverse(CastsToErase), [](Instruction *I) {
      IGC_ASSERT(I->use_empty());
      I->eraseFromParent();
    });
    IGC_ASSERT(SI->use_empty());
    SI->eraseFromParent();
    Changed = true;
  }
  return Changed;
}
} // anonymous namespace

namespace IGC {
class SplitSelectsOfAllocaPointers : public ModulePass {
public:
  static char ID;

  SplitSelectsOfAllocaPointers();

  StringRef getPassName() const override { return "SplitSelectsOfAllocaPointers"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    // Splitting a store consumer inserts an if/then/else, so the CFG is not preserved.
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<ModuleAllocaAnalysis>();
  }

  bool runOnModule(Module &M) override;
};

ModulePass *createSplitSelectsOfAllocaPointers() { return new SplitSelectsOfAllocaPointers(); }
} // namespace IGC

using namespace IGC;

// Register pass to igc-opt
#define PASS_FLAG "igc-split-selects-of-alloca-pointers"
#define PASS_DESCRIPTION "Split SELECTs of alloca pointers into per-operand loads/stores"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(SplitSelectsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(ModuleAllocaAnalysis)
IGC_INITIALIZE_PASS_END(SplitSelectsOfAllocaPointers, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char IGC::SplitSelectsOfAllocaPointers::ID = 0;

IGC::SplitSelectsOfAllocaPointers::SplitSelectsOfAllocaPointers() : ModulePass(ID) {
  initializeSplitSelectsOfAllocaPointersPass(*PassRegistry::getPassRegistry());
}

bool IGC::SplitSelectsOfAllocaPointers::runOnModule(Module &M) {
  if (!IGC_IS_FLAG_ENABLED(EnableSelectOfAllocaPtrSplit)) {
    return false;
  }

  CodeGenContext *Ctx = getAnalysis<CodeGenContextWrapper>().getCodeGenContext();
  IGC_ASSERT(Ctx != nullptr);

  // When private memory is backed by scratch space, the duplicated loads can be
  // plain speculative loads (OOB reads are hardware-masked). Otherwise they must
  // be predicated so the not-taken operand is never dereferenced -- unless
  // DisablePredicatedLoadForAllocaPtrSelectSplit forces regular loads (for
  // testing).
  //
  // The scratch-space decision is queried from ModuleAllocaAnalysis rather than
  // read from ModuleMetaData::compOpt.UseScratchSpacePrivateMemory, because that
  // field is only written by PrivateMemoryResolution, which runs after this
  // pass; before that it still holds its default (true) and would silently
  // select plain speculative loads for stateless-global private memory.
  const bool UseScratchSpacePrivateMemory = getAnalysis<ModuleAllocaAnalysis>().safeToUseScratchSpace();
  const bool CanUsePredicatedLoads = Ctx->platform.hasLSC() && Ctx->platform.LSCEnabled();

  bool UsePredicatedLoads = false;
  if (!UseScratchSpacePrivateMemory) {
    if (CanUsePredicatedLoads) {
      UsePredicatedLoads = true;
    } else if (!IGC_IS_FLAG_ENABLED(DisablePredicatedLoadForAllocaPtrSelectSplit)) {
      // Private memory in stateless global requires predicated loads, which are
      // unavailable and not explicitly disabled -> do not split.
      return false;
    }
  }

  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration()) {
      continue;
    }
    Changed |= splitSelectsOfAllocaPointers(F, UsePredicatedLoads);
  }
  return Changed;
}
