/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/CISACodeGen/SplitSelectsOfAllocaPointers.hpp"
#include "Compiler/CISACodeGen/SoAAcrossFNUtil.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/DenseSet.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/Loads.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstIterator.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Local.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/GenCodeGenModule.h"
#include "Compiler/ModuleAllocaAnalysis.hpp"
#include "common/igc_regkeys.hpp"
#include "llvmWrapper/Analysis/ValueTracking.h"

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
void splitLoadOfSelectPtr(LoadInst *LI, SelectInst *SI, bool UsePredicatedLoads, const DataLayout &DL,
                          const DenseSet<const Value *> &ExtraRoots) {
  SmallVector<Instruction *, 4> Chain;
  collectCastChain(SI, LI, Chain);
  IRBuilder<> B(LI);
  B.SetCurrentDebugLocation(LI->getDebugLoc());
  Value *Cond = SI->getCondition();
  Value *PtrTrue = IGC::SoAAcrossFNUtil::replayAddressChain(B, SI->getTrueValue(), Chain, DL, ExtraRoots);
  Value *PtrFalse = IGC::SoAAcrossFNUtil::replayAddressChain(B, SI->getFalseValue(), Chain, DL, ExtraRoots);
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
  Value *Ptr = LI->getPointerOperand();
  LI->eraseFromParent();
  RecursivelyDeleteTriviallyDeadInstructions(Ptr);
}

// Transform: store v, chain(SI)  ==> if (cond) store v, chain(A); else store v, chain(B);
void splitStoreOfSelectPtr(StoreInst *StI, SelectInst *SI, const DataLayout &DL,
                           const DenseSet<const Value *> &ExtraRoots) {
  SmallVector<Instruction *, 4> Chain;
  collectCastChain(SI, StI, Chain);
  Instruction *ThenTerm = nullptr;
  Instruction *ElseTerm = nullptr;
  SplitBlockAndInsertIfThenElse(SI->getCondition(), StI, &ThenTerm, &ElseTerm);
  {
    IRBuilder<> B(ThenTerm);
    B.SetCurrentDebugLocation(StI->getDebugLoc());
    Value *PtrTrue = IGC::SoAAcrossFNUtil::replayAddressChain(B, SI->getTrueValue(), Chain, DL, ExtraRoots);
    auto *STrue = cast<StoreInst>(StI->clone());
    STrue->setOperand(StI->getPointerOperandIndex(), PtrTrue);
    B.Insert(STrue);
  }
  {
    IRBuilder<> B(ElseTerm);
    B.SetCurrentDebugLocation(StI->getDebugLoc());
    Value *PtrFalse = IGC::SoAAcrossFNUtil::replayAddressChain(B, SI->getFalseValue(), Chain, DL, ExtraRoots);
    auto *SFalse = cast<StoreInst>(StI->clone());
    SFalse->setOperand(StI->getPointerOperandIndex(), PtrFalse);
    B.Insert(SFalse);
  }
  Value *Ptr = StI->getPointerOperand();
  StI->eraseFromParent();
  RecursivelyDeleteTriviallyDeadInstructions(Ptr);
}

// Walk from SI through bitcast/addrspacecast users, collecting the leaf
// load/store consumers. Returns false if any non-supported user is encountered
// (e.g. another SELECT, a GEP, a PHI), in which case the SI is not safe to split.
bool collectSelectLeafConsumers(SelectInst *SI, SmallVectorImpl<Instruction *> &Loads,
                                SmallVectorImpl<Instruction *> &Stores) {
  SmallVector<Instruction *, 8> Stack;
  Stack.push_back(SI);
  while (!Stack.empty()) {
    Instruction *Cur = Stack.pop_back_val();
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
        Stack.push_back(cast<Instruction>(U));
      } else {
        return false;
      }
    }
  }
  return true;
}

// Check if the duplicated loads need predication. Returns false if the load cannot be split (predication required but
// unavailable).
bool canSplitLoadOfSelectPtr(LoadInst *LI, SelectInst *SI, bool UseScratchSpacePrivateMemory,
                             bool CanUsePredicatedLoads, bool &NeedPredicatedLoads) {
  const DataLayout &DL = LI->getModule()->getDataLayout();
  // The cast chain between SI and LI consists of bitcasts/addrspacecasts only,
  // so the select operands address the same memory as the split loads would.
  Value *PtrTrue = SI->getTrueValue();
  Value *PtrFalse = SI->getFalseValue();
  NeedPredicatedLoads =
      !UseScratchSpacePrivateMemory ||
      (!isa<AllocaInst>(IGCLLVM::getUnderlyingObject(PtrTrue, DL)) &&
       !isSafeToLoadUnconditionally(PtrTrue, LI->getType(), LI->getAlign(), DL, /*ScanFrom=*/nullptr)) ||
      (!isa<AllocaInst>(IGCLLVM::getUnderlyingObject(PtrFalse, DL)) &&
       !isSafeToLoadUnconditionally(PtrFalse, LI->getType(), LI->getAlign(), DL, /*ScanFrom=*/nullptr));
  if (NeedPredicatedLoads && IGC_IS_FLAG_ENABLED(DisablePredicatedLoadForAllocaPtrSelectSplit)) {
    NeedPredicatedLoads = false;
  }
  if (NeedPredicatedLoads && !CanUsePredicatedLoads) {
    return false;
  }
  return true;
}

bool splitSelectsOfRootedPointers(Function &F, bool UseScratchSpacePrivateMemory, bool CanUsePredicatedLoads,
                                  const DenseSet<const Value *> &ExtraRoots) {
  SmallVector<SelectInst *, 16> WorkList;
  const DataLayout &DL = F.getParent()->getDataLayout();
  for (Instruction &I : instructions(F)) {
    auto *SI = dyn_cast<SelectInst>(&I);
    if (!SI || !SI->getType()->isPointerTy()) {
      continue;
    }
    auto IsRoot = [&](Value *V) {
      Value *Obj = IGCLLVM::getUnderlyingObject(V, DL);
      return isa<AllocaInst>(Obj) || ExtraRoots.contains(Obj);
    };
    if (IsRoot(SI->getTrueValue()) || IsRoot(SI->getFalseValue())) {
      WorkList.push_back(SI);
    }
  }
  bool Changed = false;
  SmallVector<Instruction *, 8> Loads;
  SmallVector<bool, 8> LoadNeedsPredication;
  SmallVector<Instruction *, 4> Stores;
  for (SelectInst *SI : WorkList) {
    Loads.clear();
    Stores.clear();
    LoadNeedsPredication.clear();
    if (!collectSelectLeafConsumers(SI, Loads, Stores)) {
      continue;
    }
    if (Loads.empty() && Stores.empty()) {
      continue;
    }

    bool CanSplit = true;
    for (Instruction *LI : Loads) {
      bool NeedPredicatedLoads = false;
      if (!canSplitLoadOfSelectPtr(cast<LoadInst>(LI), SI, UseScratchSpacePrivateMemory, CanUsePredicatedLoads,
                                   NeedPredicatedLoads)) {
        CanSplit = false;
        break;
      }
      LoadNeedsPredication.push_back(NeedPredicatedLoads);
    }
    if (!CanSplit) {
      continue;
    }

    for (auto [LI, NeedPredicatedLoads] : llvm::zip(Loads, LoadNeedsPredication)) {
      splitLoadOfSelectPtr(cast<LoadInst>(LI), SI, NeedPredicatedLoads, DL, ExtraRoots);
    }
    for (Instruction *St : Stores) {
      splitStoreOfSelectPtr(cast<StoreInst>(St), SI, DL, ExtraRoots);
    }
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
    AU.addRequired<MetaDataUtilsWrapper>();
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
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
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
  const bool UseScratchSpacePrivateMemory = getAnalysis<ModuleAllocaAnalysis>().safeToUseScratchSpace();
  const bool CanUsePredicatedLoads = Ctx->platform.hasLSC() && Ctx->platform.LSCEnabled();

  if (!UseScratchSpacePrivateMemory) {
    if (!CanUsePredicatedLoads && IGC_IS_FLAG_DISABLED(DisablePredicatedLoadForAllocaPtrSelectSplit)) {
      // Private memory in stateless global requires predicated loads, which are
      // unavailable and not explicitly disabled -> do not split.
      return false;
    }
  }

  IGCMD::MetaDataUtils *MdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  DenseSet<const Value *> RootArgs;
  if (IGC_IS_FLAG_ENABLED(EnableSOAAcrossCalls) && IGC_IS_FLAG_DISABLED(DisableSOAPromotion)) {
    SoAAcrossFNUtil::collectRootArgs(M, getAnalysisIfAvailable<GenXFunctionGroupAnalysis>(), MdUtils,
                                     /*VerifyAlloca=*/true, RootArgs);
  }

  bool Changed = false;
  for (Function &F : M) {
    if (F.isDeclaration() || MdUtils->findFunctionsInfoItem(&F) == MdUtils->end_FunctionsInfo()) {
      continue;
    }
    Changed |= splitSelectsOfRootedPointers(F, UseScratchSpacePrivateMemory, CanUsePredicatedLoads, RootArgs);
  }
  return Changed;
}
