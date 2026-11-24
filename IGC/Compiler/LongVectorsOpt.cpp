/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/SetVector.h"
#include "llvm/ADT/SmallPtrSet.h"
#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/ValueTracking.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/KnownBits.h"
#include "llvm/Support/MathExtras.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "common/LLVMWarningsPop.hpp"

#include "common/IGCIRBuilder.h"
#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/LongVectorsOpt.h"
#include "Probe/Assertion.h"

#define PASS_FLAG "igc-long-vectors-opt"
#define PASS_DESCRIPTION "Optimize long vectors"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false

using namespace llvm;

namespace IGC {
class LongVectorsOpt : public FunctionPass {
public:
  static char ID;

  LongVectorsOpt();
  ~LongVectorsOpt() {};

  virtual bool runOnFunction(Function &F) override;

  virtual void getAnalysisUsage(AnalysisUsage &AU) const override;

  virtual StringRef getPassName() const override;

private:
  bool isLongVector(Type *Ty);
  void gatherInstructions(Function &F);
  bool resolvePhi(PHINode *Phi);
  bool resolveStore(Instruction *Store);
  bool resolveExtractElement(ExtractElementInst *Extract);
  bool collectHoistable(Value *V, Instruction *Load, SmallSetVector<Instruction *, 8> &ToHoist);
  bool resolveExtractElementToLoad(ExtractElementInst *Extract);
  bool isLoadedMemoryUnchangedAt(LoadInst *Load, Instruction *Pos);
  bool resolveInsertElement(InsertElementInst *Insert);

private:
  const DataLayout *DL;
  DominatorTree *DT;
  AAResults *AA;
  SmallSetVector<PHINode *, 4> Phis;
  SmallSetVector<ExtractElementInst *, 4> ExtractElements;
  SmallSetVector<InsertElementInst *, 4> InsertElements;
  SmallSetVector<Instruction *, 4> Stores;
  SmallSetVector<Instruction *, 4> ResolvedLoads;
  // Inserts erased by resolveStore, so that resolveInsertElement skips them.
  SmallPtrSet<InsertElementInst *, 16> ErasedInserts;
  // Private-memory copies of long vectors read by variable-index extracts.
  DenseMap<Value *, AllocaInst *> VectorToAlloca;
  DenseMap<std::pair<LoadInst *, BasicBlock *>, bool> UnchangedAfterLoad;
};

IGC_INITIALIZE_PASS_BEGIN(LongVectorsOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
IGC_INITIALIZE_PASS_DEPENDENCY(AAResultsWrapperPass)
IGC_INITIALIZE_PASS_END(LongVectorsOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LongVectorsOpt::ID = 0;

////////////////////////////////////////////////////////////////////////////////
LongVectorsOpt::LongVectorsOpt() : FunctionPass(LongVectorsOpt::ID) {
  initializeLongVectorsOptPass(*PassRegistry::getPassRegistry());
}

////////////////////////////////////////////////////////////////////////////////
void LongVectorsOpt::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesCFG();
  AU.addRequired<DominatorTreeWrapperPass>();
  AU.addRequired<AAResultsWrapperPass>();
}

////////////////////////////////////////////////////////////////////////////////
StringRef LongVectorsOpt::getPassName() const { return "LongVectorsOpt"; }

////////////////////////////////////////////////////////////////////////////////
inline Type *getValueType(Instruction *Inst) {
  if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    return Store->getValueOperand()->getType();
  } else if (StoreRawIntrinsic *StoreRaw = dyn_cast<StoreRawIntrinsic>(Inst)) {
    return StoreRaw->getStoreValue()->getType();
  }
  IGC_ASSERT(isa<LoadInst>(Inst) || isa<LdRawIntrinsic>(Inst));
  return Inst->getType();
}

////////////////////////////////////////////////////////////////////////////////
inline Value *getValueOperand(Instruction *StoreOrStoreRaw) {
  if (StoreInst *Store = dyn_cast<StoreInst>(StoreOrStoreRaw)) {
    return Store->getValueOperand();
  }
  IGC_ASSERT(isa<StoreRawIntrinsic>(StoreOrStoreRaw));
  StoreRawIntrinsic *StoreRaw = cast<StoreRawIntrinsic>(StoreOrStoreRaw);
  return StoreRaw->getStoreValue();
}

////////////////////////////////////////////////////////////////////////////////
inline Value *getPointerOperand(Instruction *Inst) {
  if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    return Store->getPointerOperand();
  }
  IGC_ASSERT(isa<LoadInst>(Inst));
  return cast<LoadInst>(Inst)->getPointerOperand();
}

////////////////////////////////////////////////////////////////////////////////
inline Value *getResourceOperand(Instruction *Inst) {
  if (StoreRawIntrinsic *StoreRaw = dyn_cast<StoreRawIntrinsic>(Inst)) {
    return StoreRaw->getResourceValue();
  }
  IGC_ASSERT(isa<LdRawIntrinsic>(Inst));
  return cast<LdRawIntrinsic>(Inst)->getResourceValue();
}

////////////////////////////////////////////////////////////////////////////////
inline Value *getOffsetOperand(Instruction *Inst) {
  if (StoreRawIntrinsic *StoreRaw = dyn_cast<StoreRawIntrinsic>(Inst)) {
    return StoreRaw->getOffsetValue();
  }
  IGC_ASSERT(isa<LdRawIntrinsic>(Inst));
  return cast<LdRawIntrinsic>(Inst)->getOffsetValue();
}

////////////////////////////////////////////////////////////////////////////////
inline bool isVolatile(Instruction *Inst) {
  if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    return Store->isVolatile();
  } else if (StoreRawIntrinsic *StoreRaw = dyn_cast<StoreRawIntrinsic>(Inst)) {
    return StoreRaw->isVolatile();
  } else if (LoadInst *Load = dyn_cast<LoadInst>(Inst)) {
    return Load->isVolatile();
  }
  IGC_ASSERT(isa<LdRawIntrinsic>(Inst));
  return cast<LdRawIntrinsic>(Inst)->isVolatile();
}

////////////////////////////////////////////////////////////////////////////////
// Returns true if the memory access is neither volatile nor atomic, so it can be split into several accesses.
inline bool isSimpleAccess(Instruction *Inst) {
  if (StoreInst *Store = dyn_cast<StoreInst>(Inst)) {
    return Store->isSimple();
  } else if (LoadInst *Load = dyn_cast<LoadInst>(Inst)) {
    return Load->isSimple();
  }
  return !isVolatile(Inst);
}

////////////////////////////////////////////////////////////////////////////////
inline uint64_t getAlignment(Instruction *Inst) {
  if (isa<LoadInst>(Inst) || isa<StoreInst>(Inst)) {
    return getLoadStoreAlignment(Inst).value();
  } else if (StoreRawIntrinsic *StoreRaw = dyn_cast<StoreRawIntrinsic>(Inst)) {
    return StoreRaw->getAlignment();
  }
  IGC_ASSERT(isa<LdRawIntrinsic>(Inst));
  return cast<LdRawIntrinsic>(Inst)->getAlignment();
}

////////////////////////////////////////////////////////////////////////////////
inline bool isStateless(Instruction *Inst) { return isa<LoadInst>(Inst) || isa<StoreInst>(Inst); }

////////////////////////////////////////////////////////////////////////////////
bool isLongVectorsOptCandidate(Type *Ty, const DataLayout &DL) {
  const uint32_t ThresholdInBytes = IGC_GET_FLAG_VALUE(LongVectorsOptThreshold);
  if (!isa<FixedVectorType>(Ty) || (ThresholdInBytes >= DL.getTypeAllocSize(Ty))) {
    return false;
  }
  // Element offsets are computed as index * element size, so only byte-sized, power-of-two elements are supported.
  // Pointer elements report a scalar size of 0 and are rejected here too.
  const uint32_t ScalarSizeInBits = Ty->getScalarSizeInBits();
  return ScalarSizeInBits >= 8 && isPowerOf2_32(ScalarSizeInBits);
}

bool LongVectorsOpt::isLongVector(Type *Ty) { return isLongVectorsOptCandidate(Ty, *DL); }

////////////////////////////////////////////////////////////////////////////////
void LongVectorsOpt::gatherInstructions(Function &F) {
  for (BasicBlock &BB : F) {
    for (Instruction &I : BB) {
      if (isa<PHINode>(&I) && isLongVector(I.getType())) {
        Phis.insert(cast<PHINode>(&I));
      } else if (isa<ExtractElementInst>(&I) &&
                 isLongVector(cast<ExtractElementInst>(&I)->getVectorOperand()->getType())) {
        ExtractElements.insert(cast<ExtractElementInst>(&I));
      } else if (isa<InsertElementInst>(&I) && isLongVector(cast<InsertElementInst>(&I)->getOperand(0)->getType())) {
        InsertElements.insert(cast<InsertElementInst>(&I));
      } else if ((isa<StoreInst>(&I) || isa<StoreRawIntrinsic>(&I)) && isLongVector(getValueOperand(&I)->getType())) {
        Stores.insert(&I);
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
// An out-of-range index makes extractelement produce poison, but the element load would read outside the vector.
// Poison may be refined to any element, so the index is clamped into range.
inline Value *getInRangeIndex(IGCLLVM::IRBuilder<> &Builder, Value *Idx, uint32_t NumElements) {
  const DataLayout &DL = Builder.GetInsertBlock()->getModule()->getDataLayout();
  KnownBits Known = computeKnownBits(Idx, DL);
  if (Known.getMaxValue().ult(NumElements)) {
    return Idx;
  }

  if (!llvm::isGuaranteedNotToBePoison(Idx)) {
    Idx = Builder.CreateFreeze(Idx);
  }
  Constant *MaxIdx = ConstantInt::get(Idx->getType(), NumElements - 1);
  if (isPowerOf2_32(NumElements)) {
    return Builder.CreateAnd(Idx, MaxIdx);
  }
  return Builder.CreateSelect(Builder.CreateICmpULE(Idx, MaxIdx), Idx, MaxIdx);
}

////////////////////////////////////////////////////////////////////////////////
inline AllocaInst *createAllocaAtEntry(Function &F, Type *Ty, const Twine &Name) {
  IGCIRBuilder<> Builder(&F.getEntryBlock().front());
  auto *VecTy = cast<FixedVectorType>(Ty);
  // return Builder.CreateAlloca(Ty->getScalarType(), Builder.getInt32(VecTy->getNumElements()), VALUE_NAME(Name));
  return Builder.CreateAlloca(VecTy, nullptr, VALUE_NAME(Name));
}

////////////////////////////////////////////////////////////////////////////////
inline Instruction *createLoad(IGCLLVM::IRBuilder<> &Builder, Instruction *OrigInst, Value *Idx, Type *TypeToLoad) {
  auto *LongVecTy = cast<FixedVectorType>(getValueType(OrigInst));
  auto ScalarSizeInBytes = TypeToLoad->getScalarSizeInBits() / 8;
  // The element offset is a multiple of the element size, so this alignment holds for every index.
  uint64_t Alignment = MinAlign(getAlignment(OrigInst), ScalarSizeInBytes);
  if (isStateless(OrigInst)) {
    Value *Ptr = getPointerOperand(OrigInst);
    Ptr = Builder.CreateGEP(LongVecTy, Ptr, {Builder.getInt32(0), Idx});
    Ptr =
        Builder.CreatePointerCast(Ptr, IGCLLVM::PointerType::get(TypeToLoad, Ptr->getType()->getPointerAddressSpace()));
    return Builder.CreateAlignedLoad(TypeToLoad, Ptr, Align(Alignment));
  }
  Type *BufferTy = getResourceOperand(OrigInst)->getType();
  Type *Types[] = {TypeToLoad, BufferTy};
  GenISAIntrinsic::ID IntrinsicID = TypeToLoad->isVectorTy() ? llvm::GenISAIntrinsic::GenISA_ldrawvector_indexed
                                                             : llvm::GenISAIntrinsic::GenISA_ldraw_indexed;
  Function *LdFunc = llvm::GenISAIntrinsic::getDeclaration(OrigInst->getModule(), IntrinsicID, Types);
  Value *BaseOffset = getOffsetOperand(OrigInst);
  Value *OffsetInBuffer = Builder.CreateZExtOrTrunc(Idx, BaseOffset->getType());
  OffsetInBuffer = Builder.CreateMul(OffsetInBuffer, ConstantInt::get(BaseOffset->getType(), ScalarSizeInBytes));
  OffsetInBuffer = Builder.CreateAdd(BaseOffset, OffsetInBuffer);
  llvm::Value *Args[] = {getResourceOperand(OrigInst), OffsetInBuffer, Builder.getInt32(Alignment),
                         Builder.getInt1(isVolatile(OrigInst))};
  return Builder.CreateCall(LdFunc, Args);
}

////////////////////////////////////////////////////////////////////////////////
inline Instruction *createStore(IGCLLVM::IRBuilder<> &Builder, Instruction *OrigStoreOrStoreRaw, Value *Idx,
                                Value *Val) {
  auto *LongVecTy = cast<FixedVectorType>(getValueOperand(OrigStoreOrStoreRaw)->getType());
  auto ScalarSizeInBytes = Val->getType()->getScalarSizeInBits() / 8;
  // The element offset is a multiple of the element size, so this alignment holds for every index.
  uint64_t Alignment = MinAlign(getAlignment(OrigStoreOrStoreRaw), ScalarSizeInBytes);
  if (StoreInst *Store = dyn_cast<StoreInst>(OrigStoreOrStoreRaw)) {
    Value *Ptr = Store->getPointerOperand();
    Ptr = Builder.CreateGEP(LongVecTy, Ptr, {Builder.getInt32(0), Idx});
    Ptr = Builder.CreatePointerCast(
        Ptr, IGCLLVM::PointerType::get(Val->getType(), Ptr->getType()->getPointerAddressSpace()));
    return Builder.CreateAlignedStore(Val, Ptr, Align(Alignment));
  }
  IGC_ASSERT(isa<StoreRawIntrinsic>(OrigStoreOrStoreRaw));
  StoreRawIntrinsic *StoreRaw = cast<StoreRawIntrinsic>(OrigStoreOrStoreRaw);
  Type *BufferTy = StoreRaw->getResourceValue()->getType();
  Type *Types[] = {BufferTy, Val->getType()};
  GenISAIntrinsic::ID IntrinsicID = Val->getType()->isVectorTy() ? GenISAIntrinsic::GenISA_storerawvector_indexed
                                                                 : GenISAIntrinsic::GenISA_storeraw_indexed;
  Function *StoreFunc = llvm::GenISAIntrinsic::getDeclaration(StoreRaw->getModule(), IntrinsicID, Types);
  Value *BaseOffset = StoreRaw->getOffsetValue();
  Value *OffsetInBuffer = Builder.CreateZExtOrTrunc(Idx, BaseOffset->getType());
  OffsetInBuffer = Builder.CreateMul(OffsetInBuffer, ConstantInt::get(BaseOffset->getType(), ScalarSizeInBytes));
  OffsetInBuffer = Builder.CreateAdd(BaseOffset, OffsetInBuffer);
  llvm::Value *Args[] = {StoreRaw->getResourceValue(), OffsetInBuffer, Val, Builder.getInt32(Alignment),
                         Builder.getInt1(StoreRaw->isVolatile())};
  return Builder.CreateCall(StoreFunc, Args);
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolveStore(Instruction *Store) {
  if (!isSimpleAccess(Store)) {
    return false;
  }
  // Walk back the chain of insert-element instructions, looking for a non-constant
  // index. The other users of an insert get the vector reloaded at the store, so the
  // chain ends at an insert with a user the store doesn't dominate.
  Value *Val = getValueOperand(Store);
  SmallVector<InsertElementInst *, 16> Chain;
  InsertElementInst *ChainStart = nullptr;
  User *ChainUser = Store;
  while (InsertElementInst *Insert = dyn_cast<InsertElementInst>(Val)) {
    if (any_of(Insert->uses(), [&](Use &U) { return U.getUser() != ChainUser && !DT->dominates(Store, U); })) {
      break;
    }
    if (!isa<ConstantInt>(Insert->getOperand(2))) {
      ChainStart = Insert;
    }
    Chain.push_back(Insert);
    ChainUser = Insert;
    Val = Insert->getOperand(0);
  }
  if (ChainStart == nullptr) {
    return false;
  }
  while (Chain.back() != ChainStart) {
    Chain.pop_back();
  }

  IGCLLVM::IRBuilder<> Builder(Store);
  // Store the base vector first, then the elements in the original insert order, so later inserts overwrite earlier
  // ones at the same index.
  if (!isa<UndefValue>(ChainStart->getOperand(0))) {
    createStore(Builder, Store, Builder.getInt32(0), ChainStart->getOperand(0));
  }
  for (InsertElementInst *Insert : llvm::reverse(Chain)) {
    createStore(Builder, Store, Insert->getOperand(2), Insert->getOperand(1));
    if (!Insert->hasOneUse()) {
      Value *Tmp = createLoad(Builder, Store, Builder.getInt32(0), Insert->getType());
      Insert->replaceAllUsesWith(Tmp);
    }
  }
  Store->eraseFromParent();
  for (InsertElementInst *Insert : Chain) {
    ErasedInserts.insert(Insert);
    Insert->eraseFromParent();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Returns true if the load reads memory that can't be written, so it may be moved past stores.
inline bool isReadOnlyLoad(Instruction *LoadInstr) {
  if (!LoadInstr || !(isa<LoadInst>(LoadInstr) || isa<LdRawIntrinsic>(LoadInstr)) || !isSimpleAccess(LoadInstr)) {
    return false;
  }
  Value *BufPtr = GetBufferOperand(LoadInstr);
  uint32_t Addrspace = BufPtr->getType()->getPointerAddressSpace();
  if (isStatefulAddrSpace(Addrspace)) {
    auto BufType = GetBufferType(Addrspace);
    BufferAccessType Access = getDefaultAccessType(BufType);
    return (Access == BufferAccessType::ACCESS_READ);
  }
  return Addrspace == ADDRESS_SPACE_CONSTANT;
}

///////////////////////////////////////////////////////////////////////////////
// Collects the instructions computing V that are not available at the load, operands first, so that they can be
// cloned before the load. Fails if one of them may access memory or trap, is a PHI, or depends on the load.
bool LongVectorsOpt::collectHoistable(Value *V, Instruction *Load, SmallSetVector<Instruction *, 8> &ToHoist) {
  auto NeedsHoisting = [&](Value *Val) {
    Instruction *I = dyn_cast<Instruction>(Val);
    return I && (I == Load || !DT->dominates(I, Load)) && !ToHoist.count(I);
  };
  auto CanHoist = [&](Instruction *I) {
    return I != Load && !isa<PHINode>(I) && !isa<FreezeInst>(I) && !I->mayReadOrWriteMemory() &&
           isSafeToSpeculativelyExecute(I);
  };
  if (!NeedsHoisting(V)) {
    return true;
  }
  // Depth-first walk over the operands. Each stack entry holds an instruction and the index of its next operand to
  // visit; the instruction is added to ToHoist once all its operands are done.
  SmallVector<std::pair<Instruction *, unsigned>, 8> Stack;
  Stack.push_back({cast<Instruction>(V), 0});
  if (!CanHoist(Stack.back().first)) {
    return false;
  }
  while (!Stack.empty()) {
    Instruction *I = Stack.back().first;
    unsigned &NextOperand = Stack.back().second;
    if (NextOperand == I->getNumOperands()) {
      ToHoist.insert(I);
      Stack.pop_back();
      continue;
    }
    Value *Op = I->getOperand(NextOperand++);
    if (!NeedsHoisting(Op)) {
      continue;
    }
    Instruction *OpInst = cast<Instruction>(Op);
    if (!CanHoist(OpInst)) {
      return false;
    }
    Stack.push_back({OpInst, 0});
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Returns true if nothing can write the memory Load reads between Load and Pos, which Load dominates.
// Pos may run many times after Load, e.g. in a loop, so no block that reaches Pos's block without passing through
// Load's block, where Load runs again, may write it at all. That includes Pos's block itself.
bool LongVectorsOpt::isLoadedMemoryUnchangedAt(LoadInst *Load, Instruction *Pos) {
  IGC_ASSERT(DT->dominates(Load, Pos));
  MemoryLocation Loc = MemoryLocation::get(Load);
  auto MayWrite = [&](Instruction &I) { return I.mayWriteToMemory() && isModSet(AA->getModRefInfo(&I, Loc)); };
  BasicBlock *LoadBB = Load->getParent();
  BasicBlock *PosBB = Pos->getParent();
  if (PosBB == LoadBB) {
    return none_of(make_range(std::next(Load->getIterator()), Pos->getIterator()), MayWrite);
  }
  if (auto It = UnchangedAfterLoad.find({Load, PosBB}); It != UnchangedAfterLoad.end()) {
    return It->second;
  }
  bool Unchanged = none_of(make_range(std::next(Load->getIterator()), LoadBB->end()), MayWrite);
  SmallVector<BasicBlock *, 8> Worklist = {PosBB};
  SmallPtrSet<BasicBlock *, 16> Visited = {PosBB};
  while (Unchanged && !Worklist.empty()) {
    BasicBlock *BB = Worklist.pop_back_val();
    Unchanged = none_of(*BB, MayWrite);
    for (BasicBlock *Pred : predecessors(BB)) {
      if (Pred != LoadBB && Visited.insert(Pred).second) {
        Worklist.push_back(Pred);
      }
    }
  }
  UnchangedAfterLoad[{Load, PosBB}] = Unchanged;
  return Unchanged;
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolveExtractElementToLoad(ExtractElementInst *Extract) {
  Instruction *LoadInstr = dyn_cast<Instruction>(Extract->getVectorOperand());
  if (!LoadInstr || !(isa<LoadInst>(LoadInstr) || isa<LdRawIntrinsic>(LoadInstr)) || !isSimpleAccess(LoadInstr)) {
    return false;
  }
  IGCLLVM::IRBuilder<> Builder(LoadInstr);

  bool IsReadOnly = isReadOnlyLoad(LoadInstr);
  // The element load is created at the original load if the index must be available there.
  // Otherwise, the index is hoisted to the load if it is cheap and safe to compute there.
  // If the memory is readonly, or nothing can write it before the extract, the load may be moved to the extract.
  Value *Idx = Extract->getIndexOperand();
  SmallSetVector<Instruction *, 8> ToHoist;
  if (Instruction *IdxInst = dyn_cast<Instruction>(Idx)) {
    if (!DT->dominates(IdxInst, LoadInstr)) {
      if (IsReadOnly || (isStateless(LoadInstr) && isLoadedMemoryUnchangedAt(cast<LoadInst>(LoadInstr), Extract))) {
        Builder.SetInsertPoint(Extract);
      } else if (collectHoistable(IdxInst, LoadInstr, ToHoist)) {
        // The index is cheap and safe to compute at the load, so compute it again there.
        ValueToValueMapTy HoistedValues;
        for (Instruction *I : ToHoist) {
          Instruction *Clone = I->clone();
          IGCLLVM::insertBefore(Clone, LoadInstr);
          RemapInstruction(Clone, HoistedValues, RF_IgnoreMissingLocals | RF_NoModuleLevelChanges);
          HoistedValues[I] = Clone;
        }
        Idx = HoistedValues[IdxInst];
      } else {
        // TODO: if there are no aliasing stores between the index instruction and the load instruction, we can
        // create the load at the index instruction.
        return false;
      }
    }
  }
  // An out-of-range constant index yields poison; don't turn it into an out-of-bounds load.
  if (ConstantInt *ConstIdx = dyn_cast<ConstantInt>(Idx)) {
    if (ConstIdx->getValue().uge(cast<FixedVectorType>(LoadInstr->getType())->getNumElements())) {
      return false;
    }
  }
  Value *Elem = createLoad(Builder, LoadInstr, Idx, Extract->getType());
  Extract->replaceAllUsesWith(Elem);
  Extract->eraseFromParent();
  // The original index computation is left unused when the extract was its only user.
  for (Instruction *I : llvm::reverse(ToHoist)) {
    if (I->use_empty() && !(isa<ExtractElementInst>(I) && ExtractElements.count(cast<ExtractElementInst>(I)))) {
      I->eraseFromParent();
    }
  }
  ResolvedLoads.insert(LoadInstr);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Splits an extract from a long load into an element load. Otherwise, a variable-index extract reads its element from
// a private-memory copy of the vector, stored once right after the vector is defined. A constant-index extract is
// left alone: it reads the element directly from registers.
bool LongVectorsOpt::resolveExtractElement(ExtractElementInst *Extract) {
  if (resolveExtractElementToLoad(Extract)) {
    return true;
  }
  Value *Idx = Extract->getIndexOperand();
  if (isa<ConstantInt>(Idx)) {
    return false;
  }
  Value *Vec = Extract->getVectorOperand();
  auto *VecTy = cast<FixedVectorType>(Vec->getType());
  AllocaInst *&Alloca = VectorToAlloca[Vec];
  if (!Alloca) {
    Alloca = createAllocaAtEntry(*Extract->getFunction(), VecTy, VALUE_NAME(Vec->getName()));
    // Arguments and constants are available at the entry, right after the alloca.
    Instruction *VecInst = dyn_cast<Instruction>(Vec);
    IGCLLVM::IRBuilder<> Builder(VecInst ? VecInst->getNextNode() : Alloca->getNextNode());
    Builder.CreateStore(Vec, Alloca);
  }
  IGCLLVM::IRBuilder<> Builder(Extract);
  Idx = getInRangeIndex(Builder, Idx, VecTy->getNumElements());
  Value *Ptr = Builder.CreateGEP(VecTy, Alloca, {Builder.getInt32(0), Idx});
  Value *Elem = Builder.CreateLoad(Extract->getType(), Ptr);
  Extract->replaceAllUsesWith(Elem);
  Extract->eraseFromParent();
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Moves a long vector built by an insertelement chain with a variable index to private memory. The chain is split
// into element stores by resolveStore and its extracts into element loads by resolveExtractElement. Chains with
// constant indices only stay in registers.
bool LongVectorsOpt::resolveInsertElement(InsertElementInst *Insert) {
  // Inserts already split by resolveStore are gone. An insert used only by other inserts is reached by resolveStore
  // from the end of the chain; one left with other users after that gets its own slot.
  if (ErasedInserts.count(Insert) || all_of(Insert->users(), [](User *U) { return isa<InsertElementInst>(U); })) {
    return false;
  }
  // Only insert chains with non-constant indices are processed.
  bool HasNonConstantIndex = false;
  InsertElementInst *CurrentInsert = Insert;
  while (CurrentInsert) {
    if (!isa<ConstantInt>(CurrentInsert->getOperand(2))) {
      HasNonConstantIndex = true;
      break;
    }
    Value *Vec = CurrentInsert->getOperand(0);
    CurrentInsert = dyn_cast<InsertElementInst>(Vec);
  }
  if (!HasNonConstantIndex) {
    return false;
  }
  AllocaInst *Alloca = createAllocaAtEntry(*Insert->getFunction(), Insert->getType(), VALUE_NAME(Insert->getName()));
  IGCLLVM::IRBuilder<> Builder(Insert->getNextNode());
  StoreInst *NewStore = Builder.CreateStore(Insert, Alloca);
  LoadInst *NewLoad = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca);
  Insert->replaceUsesWithIf(NewLoad, [&](Use &U) { return U.getUser() != NewStore; });
  resolveStore(NewStore);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolvePhi(PHINode *Phi) {
  AllocaInst *Alloca = createAllocaAtEntry(*Phi->getFunction(), Phi->getType(), VALUE_NAME(Phi->getName()));
  for (unsigned I = 0; I < Phi->getNumOperands(); ++I) {
    IGCLLVM::IRBuilder<> Builder(Phi->getIncomingBlock(I)->getTerminator());
    Value *Source = Phi->getOperand(I);
    StoreInst *NewStore = Builder.CreateStore(Source, Alloca);
    Stores.insert(NewStore);
  }
  IGCLLVM::IRBuilder<> Builder(Phi);
  BasicBlock *Block = Builder.GetInsertBlock();
  Builder.SetInsertPoint(IGCLLVM::getFirstNonPHI(Block));
  Value *NewLoad = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca);
  Phi->replaceAllUsesWith(NewLoad);
  Phi->eraseFromParent();
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::runOnFunction(Function &F) {
  DL = &F.getParent()->getDataLayout();
  DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();
  AA = &getAnalysis<AAResultsWrapperPass>().getAAResults();
  Phis.clear();
  ExtractElements.clear();
  InsertElements.clear();
  Stores.clear();
  ResolvedLoads.clear();
  ErasedInserts.clear();
  VectorToAlloca.clear();
  UnchangedAfterLoad.clear();

  gatherInstructions(F);
  bool Modified = false;
  for (PHINode *Phi : Phis) {
    Modified |= resolvePhi(Phi);
  }
  // Resolve store instructions that write long vectors that are created with insertelement chains
  // that have at least one non-constant index.
  for (Instruction *Store : Stores) {
    Modified |= resolveStore(Store);
  }
  for (InsertElementInst *Insert : llvm::reverse(InsertElements)) {
    Modified |= resolveInsertElement(Insert);
  }
  for (ExtractElementInst *Extract : ExtractElements) {
    Modified |= resolveExtractElement(Extract);
  }
  // Remove the long loads that are no longer used after splitting their extracts.
  for (Instruction *Load : ResolvedLoads) {
    if (Load->use_empty()) {
      Load->eraseFromParent();
    }
  }

  return Modified;
}

///////////////////////////////////////////////////////////////////////////////
Pass *createLongVectorsOptPass() { return new LongVectorsOpt(); }

} // namespace IGC
