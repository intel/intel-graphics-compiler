/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/SetVector.h"
#include "llvm/IR/Dominators.h"
#include "llvm/Support/MathExtras.h"
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
  bool resolveInsertElement(InsertElementInst *Insert);

private:
  const DataLayout *DL;
  DominatorTree *DT;
  SmallSetVector<PHINode *, 4> Phis;
  SmallSetVector<ExtractElementInst *, 4> ExtractElements;
  SmallSetVector<InsertElementInst *, 4> InsertElements;
  SmallSetVector<Instruction *, 4> Stores;
  SmallSetVector<Instruction *, 4> ResolvedLoads;
  std::map<Instruction *, AllocaInst *> InstructionToAlloca;
};

IGC_INITIALIZE_PASS_BEGIN(LongVectorsOpt, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(DominatorTreeWrapperPass)
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
inline Instruction *storeChunk(IGCLLVM::IRBuilder<> &Builder, Instruction *OrigStoreOrStoreRaw, uint32_t Idx,
                               auto Chunk) {
  IGC_ASSERT(!Chunk.empty());
  Value *Val = Chunk[0];
  if (Chunk.size() > 1) {
    auto ChunkVecTy = IGCLLVM::FixedVectorType::get(Val->getType()->getScalarType(), Chunk.size());
    Val = UndefValue::get(ChunkVecTy);
    for (uint32_t I = 0; I < Chunk.size(); ++I) {
      Val = Builder.CreateInsertElement(Val, Chunk[I], Builder.getInt32(I));
    }
  }
  return createStore(Builder, OrigStoreOrStoreRaw, Builder.getInt32(Idx), Val);
}

///////////////////////////////////////////////////////////////////////////////
// Stores the elements as chunks of up to ChunkSize consecutive indices.
inline Instruction *storeElementsInChunks(IGCLLVM::IRBuilder<> &Builder, Instruction *OrigStoreOrStoreRaw,
                                          const std::map<uint32_t, Value *> &Elements) {
  constexpr uint32_t ChunkSize = 4;
  uint32_t FirstIdx = 0;
  uint32_t PrevIdx = 0;
  Instruction *LastStore = nullptr;
  SmallVector<Value *, ChunkSize> Chunk;
  for (auto IdxAndElt : Elements) {
    auto [Idx, Elt] = IdxAndElt;
    if (!Chunk.empty() && (Chunk.size() == ChunkSize || Idx != (PrevIdx + 1))) {
      LastStore = storeChunk(Builder, OrigStoreOrStoreRaw, FirstIdx, Chunk);
      Chunk.clear();
    }
    if (Chunk.empty()) {
      FirstIdx = Idx;
    }
    Chunk.push_back(Elt);
    PrevIdx = Idx;
  }
  if (!Chunk.empty()) {
    LastStore = storeChunk(Builder, OrigStoreOrStoreRaw, FirstIdx, Chunk);
  }
  return LastStore;
};

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolveStore(Instruction *Store) {
  if (!isSimpleAccess(Store)) {
    return false;
  }
  // Collect the insertelement chain feeding the store, starting from the last insert. An insert with other users
  // must stay, so the chain ends there and that insert is stored as a whole vector.
  SmallVector<InsertElementInst *, 16> Chain;
  Value *Val = getValueOperand(Store);
  while (InsertElementInst *Insert = dyn_cast<InsertElementInst>(Val)) {
    if (!Insert->hasOneUse()) {
      break;
    }
    Chain.push_back(Insert);
    Val = Insert->getOperand(0);
  }
  if (Chain.empty()) {
    return false;
  }

  IGCLLVM::IRBuilder<> Builder(Store);
  // Store the base vector first, then the elements in the original insert order, so later inserts overwrite earlier
  // ones at the same index.
  if (!isa<UndefValue>(Val)) {
    createStore(Builder, Store, Builder.getInt32(0), Val);
  }
  for (InsertElementInst *Insert : llvm::reverse(Chain)) {
    createStore(Builder, Store, Insert->getOperand(2), Insert->getOperand(1));
  }
  Store->eraseFromParent();
  for (InsertElementInst *Insert : Chain) {
    Insert->eraseFromParent();
  }
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolveExtractElement(ExtractElementInst *Extract) {
  // Let's assume that the loads-merging passes work ok and generate scalar loads.
  Instruction *LoadInstr = dyn_cast<Instruction>(Extract->getVectorOperand());
  if (!LoadInstr || !(isa<LoadInst>(LoadInstr) || isa<LdRawIntrinsic>(LoadInstr)) || !isSimpleAccess(LoadInstr)) {
    return false;
  }
  // The element load is created at the original load, so the index must be available there.
  Value *Idx = Extract->getIndexOperand();
  if (Instruction *IdxInst = dyn_cast<Instruction>(Idx)) {
    if (!DT->dominates(IdxInst, LoadInstr)) {
      return false;
    }
  }
  // An out-of-range constant index yields poison; don't turn it into an out-of-bounds load.
  if (ConstantInt *ConstIdx = dyn_cast<ConstantInt>(Idx)) {
    if (ConstIdx->getValue().uge(cast<FixedVectorType>(LoadInstr->getType())->getNumElements())) {
      return false;
    }
  }
  IGCLLVM::IRBuilder<> Builder(LoadInstr);
  Value *Elem = createLoad(Builder, LoadInstr, Idx, Extract->getType());
  Extract->replaceAllUsesWith(Elem);
  Extract->eraseFromParent();
  ResolvedLoads.insert(LoadInstr);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
// Moves a long vector built by an insertelement chain to private memory. The chain is split into element stores by
// resolveStore and its extracts into element loads by resolveExtractElement.
bool LongVectorsOpt::resolveInsertElement(InsertElementInst *Insert) {
  // Only the last insert of a chain is processed, resolveStore walks the rest.
  if (Insert->hasOneUse() && isa<InsertElementInst>(*Insert->user_begin())) {
    return false;
  }
  // Inserts used only by stores are already handled by resolveStore.
  if (all_of(Insert->users(), [&](User *U) { return Stores.count(cast<Instruction>(U)) > 0; })) {
    return false;
  }
  AllocaInst *Alloca = createAllocaAtEntry(*Insert->getFunction(), Insert->getType(), VALUE_NAME(Insert->getName()));
  InstructionToAlloca[Insert] = Alloca;
  IGCLLVM::IRBuilder<> Builder(Insert->getNextNode());
  StoreInst *NewStore = Builder.CreateStore(Insert, Alloca);
  LoadInst *NewLoad = Builder.CreateLoad(Alloca->getAllocatedType(), Alloca);
  Insert->replaceUsesWithIf(NewLoad, [&](Use &U) { return U.getUser() != NewStore; });
  Stores.insert(NewStore);
  return true;
}

///////////////////////////////////////////////////////////////////////////////
bool LongVectorsOpt::resolvePhi(PHINode *Phi) {
  AllocaInst *Alloca = createAllocaAtEntry(*Phi->getFunction(), Phi->getType(), VALUE_NAME(Phi->getName()));
  InstructionToAlloca[Phi] = Alloca;
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
  Phis.clear();
  ExtractElements.clear();
  InsertElements.clear();
  Stores.clear();
  ResolvedLoads.clear();
  InstructionToAlloca.clear();

  gatherInstructions(F);
  bool Modified = false;
  for (PHINode *Phi : Phis) {
    Modified |= resolvePhi(Phi);
  }
  // Inserts must be processed before stores: they add new stores, and resolveStore erases the inserts it splits.
  for (InsertElementInst *Insert : InsertElements) {
    Modified |= resolveInsertElement(Insert);
  }
  for (Instruction *Store : Stores) {
    Modified |= resolveStore(Store);
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
