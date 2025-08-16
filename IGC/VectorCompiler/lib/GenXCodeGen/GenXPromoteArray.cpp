/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXPromoteArray
/// --------------------
///
/// GenXPromoteArray is an optimization pass that converts load/store
/// from an allocated private array into vector loads/stores followed by
/// read-region and write-region.  Then we can apply standard llvm optimization
/// to promote the entire array into virtual registers, and remove those
/// loads and stores
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "GenXUtil.h"
#include "GenXVisa.h"

#include "vc/Support/BackendConfig.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/General/STLExtras.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/Type.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Support/TypeSize.h"
#include <llvmWrapper/ADT/Optional.h>

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"

#include <algorithm>
#include <queue>
#include <vector>

#define DEBUG_TYPE "genx-promote-array"

using namespace llvm;
using namespace genx;

static cl::opt<std::size_t> SingleAllocaLimitOpt(
    "vc-promote-array-single-alloca-limit",
    cl::desc("max size of a sindle promoted alloca in bytes"),
    cl::init(96 * defaultGRFByteSize), cl::Hidden);

static cl::opt<std::size_t>
    TotalAllocaLimitOpt("vc-promote-array-total-alloca-limit",
                        cl::desc("max total size of promoted allocas in bytes"),
                        cl::init(256 * defaultGRFByteSize), cl::Hidden);

namespace {

/// @brief  GenXPromoteArray pass is used for lowering the allocas identified
/// while visiting the alloca instructions and then inserting insert/extract
/// elements instead of load stores. This allows us to store the data in
/// registers instead of propagating it to scratch space.
class GenXPromoteArray : public FunctionPass,
                         public InstVisitor<GenXPromoteArray> {
public:
  static char ID;
  explicit GenXPromoteArray() : FunctionPass(ID) {}

  StringRef getPassName() const override { return "GenXPromoteArray"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(Function &F) override;

  void visitAllocaInst(AllocaInst &I);
  void visitStore(StoreInst &I);
  void visitLoad(LoadInst &I);

  unsigned int extractAllocaSize(AllocaInst *pAlloca);

private:
  AllocaInst *createVectorForAlloca(AllocaInst *pAlloca, Type *pBaseType);
  void handleAllocaInst(AllocaInst *pAlloca);

  void selectAllocasToHandle();
  bool isAllocaPromotable(AllocaInst &pAlloca);

  void replaceAggregatedStore(StoreInst *SI);
  void replaceAggregatedLoad(LoadInst *LI);

  IGCLLVM::FixedVectorType &getVectorTypeForAlloca(AllocaInst &Alloca,
                                                   Type &ElemTy) const;

  bool checkTypes(Type *CurBaseTy, Type *NewTy) const;
  bool checkAllocaUsesInternal(Instruction *I, Type *CurBaseTy,
                               bool NeedCheckTypes) const;
  bool checkPtrToIntCandidate(PtrToIntInst *PTI, Type *CurBaseTy) const;

  const DataLayout *DL = nullptr;
  LLVMContext *Ctx = nullptr;
  Function *Func = nullptr;

  SmallVector<Instruction *, 8> AggregatesToReplace;
  std::vector<AllocaInst *> AllocasToPrivMem;
  bool ForcePromotion = false;
  bool LargeAllocasWereLeft = false;
};
} // namespace

// Register pass to igc-opt
namespace llvm {
void initializeGenXPromoteArrayPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXPromoteArray, "GenXPromoteArray", "GenXPromoteArray",
                      false, false)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(GenXPromoteArray, "GenXPromoteArray", "GenXPromoteArray",
                    false, false)

char GenXPromoteArray::ID = 0;

FunctionPass *llvm::createGenXPromoteArrayPass() {
  initializeGenXPromoteArrayPass(*PassRegistry::getPassRegistry());
  return new GenXPromoteArray();
}

namespace {
// The class preserves index into a vector and the size of an element
// of this vector.
// The idea is that vector can change throughout bitcasts and its index
// and element size should change correspondingly.
// A product of Index and ElementSizeInBits gives an offset in bits of
// a considered element in a considered vector.
struct GenericVectorIndex {
  Value *Index;
  unsigned ElementSizeInBits;

  unsigned getElementSizeInBytes() const {
    return ElementSizeInBits / genx::ByteBits;
  }

  template <typename FolderT = ConstantFolder>
  void adjust(Type *Ty, IRBuilder<FolderT> &IRB);
};

class TransposeHelper {
public:
  void handleAllocaSources(Instruction &Inst, GenericVectorIndex Idx);
  void handleGEPInst(GetElementPtrInst *GEP, GenericVectorIndex Idx);
  void handleBCInst(BitCastInst &BC, GenericVectorIndex Idx);
  void handlePTIInst(PtrToIntInst &BC, GenericVectorIndex Idx);
  void handlePHINode(PHINode *Phi, GenericVectorIndex Idx,
                     BasicBlock *IncomingBB);

  void handleLoadInst(LoadInst *Load, GenericVectorIndex Idx);
  void handleStoreInst(StoreInst *Store, GenericVectorIndex Idx);
  void handleGather(IntrinsicInst *II, GenericVectorIndex Idx,
                    unsigned MaskIndex, unsigned ValueIndex);
  void handleScatter(IntrinsicInst *II, GenericVectorIndex Idx,
                     unsigned MaskIndex, unsigned ValueIndex);
  void handleLifetimeStart(IntrinsicInst *II, GenericVectorIndex Idx);
  void handleLifetimeEnd(IntrinsicInst *II, GenericVectorIndex Idx);
  void EraseDeadCode();

  TransposeHelper(AllocaInst *AI, const DataLayout *Layout)
      : VectorAlloca(AI), DL(Layout) {}

  AllocaInst *VectorAlloca;

private:
  // Loads vector and casts it if necessary.
  // \p CastTy describes vector element type to cast to.
  template <typename FolderT = ConstantFolder>
  static Instruction *loadAndCastVector(AllocaInst *VecAlloca, Type *CastTy,
                                        IRBuilder<FolderT> &IRB);

  // Casts \p NewValue if its type doesn't correspond to allocated vector type,
  // then stores the value.
  template <typename FolderT = ConstantFolder>
  static Instruction *castAndStoreVector(AllocaInst *VecAlloca, Value *NewValue,
                                         IRBuilder<FolderT> &IRB);

  bool VectorIndex = false;
  std::vector<Instruction *> ToBeRemoved;
  ValueMap<PHINode *, PHINode *> PhiReplacement;

  const DataLayout *DL = nullptr;
};

Type *getBaseType(Type *Ty, Type *BaseTy) {
  while (Ty->isStructTy() || Ty->isArrayTy() || Ty->isVectorTy()) {
    if (Ty->isStructTy()) {
      int NumElements = Ty->getStructNumElements();
      for (int I = 0; I < NumElements; ++I) {
        auto *StructElemTy = Ty->getStructElementType(I);
        auto *StructElemBaseTy = getBaseType(StructElemTy, BaseTy);
        // can support only homogeneous structures
        if (BaseTy && (!StructElemBaseTy || StructElemBaseTy != BaseTy))
          return nullptr;
        BaseTy = StructElemBaseTy;
      }
      return BaseTy;
    } else if (Ty->isArrayTy()) {
      Ty = Ty->getArrayElementType();
    } else if (Ty->isVectorTy()) {
      Ty = cast<VectorType>(Ty)->getElementType();
    }
  }
  if (vc::isFunctionPointerType(Ty))
    Ty = IntegerType::getInt64Ty(Ty->getContext());
  return Ty;
}

template <typename FolderT>
void GenericVectorIndex::adjust(Type *Ty, IRBuilder<FolderT> &IRB) {
  auto *BaseTy = getBaseType(Ty, nullptr);
  IGC_ASSERT_EXIT(BaseTy);
  unsigned NewElementSizeInBits = BaseTy->getScalarSizeInBits();
  if (NewElementSizeInBits == ElementSizeInBits ||
      vc::isFunctionPointerType(BaseTy))
    return;
  if (NewElementSizeInBits < ElementSizeInBits) {
    IGC_ASSERT_MESSAGE(ElementSizeInBits % NewElementSizeInBits == 0,
                       "New element size is not a divisor of the current one");
    Constant *Scale = IRB.getInt32(ElementSizeInBits / NewElementSizeInBits);
    if (Index->getType()->isVectorTy()) {
      auto Width =
          cast<IGCLLVM::FixedVectorType>(Index->getType())->getNumElements();
      Scale = ConstantVector::getSplat(IGCLLVM::getElementCount(Width), Scale);
    }
    Index = IRB.CreateMul(Index, Scale);
  } else {
    IGC_ASSERT_MESSAGE(NewElementSizeInBits % ElementSizeInBits == 0,
                       "New element size is not a multiple of the current one");
    Constant *Scale = IRB.getInt32(NewElementSizeInBits / ElementSizeInBits);
    if (Index->getType()->isVectorTy()) {
      auto Width =
          cast<IGCLLVM::FixedVectorType>(Index->getType())->getNumElements();
      Scale = ConstantVector::getSplat(IGCLLVM::getElementCount(Width), Scale);
    }
    Index = IRB.CreateUDiv(Index, Scale);
  }
  ElementSizeInBits = NewElementSizeInBits;
}

template <typename FolderT>
Instruction *TransposeHelper::loadAndCastVector(AllocaInst *VecAlloca,
                                                Type *CastTy,
                                                IRBuilder<FolderT> &IRB) {
  auto *LoadVecAlloca =
      IRB.CreateLoad(VecAlloca->getAllocatedType(), VecAlloca);
  auto *AllocatedElemTy = LoadVecAlloca->getType()->getScalarType();
  if (AllocatedElemTy == CastTy || vc::isFunctionPointerType(CastTy))
    return LoadVecAlloca;
  auto AllocatedWidth = cast<IGCLLVM::FixedVectorType>(LoadVecAlloca->getType())
                            ->getNumElements();
  IGC_ASSERT(AllocatedElemTy->getScalarSizeInBits() >=
             CastTy->getScalarSizeInBits());
  IGC_ASSERT(CastTy->getScalarSizeInBits());
  IGC_ASSERT((AllocatedElemTy->getScalarSizeInBits() %
              CastTy->getScalarSizeInBits()) == 0);
  auto CastedWidth = AllocatedWidth * (AllocatedElemTy->getScalarSizeInBits() /
                                       CastTy->getScalarSizeInBits());
  auto *CastVTy = IGCLLVM::FixedVectorType::get(CastTy, CastedWidth);
  auto *Cast = IRB.CreateBitCast(LoadVecAlloca, CastVTy, ".post.load.bc");
  return cast<Instruction>(Cast);
}

template <typename FolderT>
Instruction *TransposeHelper::castAndStoreVector(AllocaInst *VecAlloca,
                                                 Value *NewValue,
                                                 IRBuilder<FolderT> &IRB) {
  auto *CastedValue = NewValue;
  if (VecAlloca->getAllocatedType() != NewValue->getType())
    CastedValue = IRB.CreateBitCast(NewValue, VecAlloca->getAllocatedType(),
                                    NewValue->getName() + ".pre.store.bc");
  return IRB.CreateStore(CastedValue, VecAlloca);
}

void TransposeHelper::EraseDeadCode() {
  for (Instruction *I : ToBeRemoved)
    I->dropAllReferences();
  for (Instruction *I : ToBeRemoved)
    I->eraseFromParent();
}

void TransposeHelper::handleBCInst(BitCastInst &BC, GenericVectorIndex Idx) {
  ToBeRemoved.push_back(&BC);
  handleAllocaSources(BC, Idx);
}

void TransposeHelper::handlePTIInst(PtrToIntInst &PTI, GenericVectorIndex Idx) {
  IGC_ASSERT(PTI.hasOneUse());
  IGC_ASSERT(isa<InsertElementInst>(PTI.user_back()));
  IRBuilder<> IRB(&PTI);

  auto *Insert = PTI.user_back();
  auto *CastedIdx = IRB.CreateZExt(Idx.Index, PTI.getType(), PTI.getName());
  auto *Scale =
      ConstantInt::get(CastedIdx->getType(), Idx.getElementSizeInBytes());
  auto *Mul = IRB.CreateMul(CastedIdx, Scale);

  PTI.replaceAllUsesWith(Mul);
  PTI.eraseFromParent();

  IGC_ASSERT(Insert->hasOneUse());
  IGC_ASSERT(isa<ShuffleVectorInst>(Insert->user_back()));
  auto *Shuffle = Insert->user_back();

  IGC_ASSERT(Shuffle->hasOneUse());
  IGC_ASSERT(isa<BinaryOperator>(Shuffle->user_back()));
  auto *BinOp = Shuffle->user_back();
  auto *Next = BinOp;

  if (Next->hasOneUse() && isa<IntToPtrInst>(Next->user_back())) {
    Next = Next->user_back();
    ToBeRemoved.push_back(Next);
  }

  handleAllocaSources(*Next, {BinOp, genx::ByteBits});
}

void TransposeHelper::handleAllocaSources(Instruction &Inst,
                                          GenericVectorIndex Idx) {
  SmallVector<Value *, 10> Users{Inst.user_begin(), Inst.user_end()};

  for (auto *User : Users) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(User)) {
      handleGEPInst(GEP, Idx);
    } else if (auto *BC = dyn_cast<BitCastInst>(User)) {
      handleBCInst(*BC, Idx);
    } else if (auto *PTI = dyn_cast<PtrToIntInst>(User)) {
      handlePTIInst(*PTI, Idx);
    } else if (auto *Store = dyn_cast<StoreInst>(User)) {
      handleStoreInst(Store, Idx);
    } else if (auto *Load = dyn_cast<LoadInst>(User)) {
      handleLoadInst(Load, Idx);
    } else if (auto *Phi = dyn_cast<PHINode>(User)) {
      handlePHINode(Phi, Idx, Inst.getParent());
    } else if (auto *II = dyn_cast<IntrinsicInst>(User)) {
      switch (vc::getAnyIntrinsicID(II)) {
      case Intrinsic::lifetime_start:
        handleLifetimeStart(II, Idx);
        break;
      case Intrinsic::lifetime_end:
        handleLifetimeEnd(II, Idx);
        break;
      case Intrinsic::masked_gather:
        handleGather(II, Idx, 2, 3);
        break;
      case Intrinsic::masked_scatter:
        handleScatter(II, Idx, 3, 0);
        break;
      case GenXIntrinsic::genx_svm_gather:
        handleGather(II, Idx, 0, 3);
        break;
      case GenXIntrinsic::genx_svm_scatter:
        handleScatter(II, Idx, 0, 3);
        break;
      default:
        break;
      }
    }
  }
}

void TransposeHelper::handleGEPInst(GetElementPtrInst *GEP,
                                    GenericVectorIndex Idx) {
  ToBeRemoved.push_back(GEP);
  IRBuilder<> IRB(GEP);
  Idx.adjust(GEP->getSourceElementType(), IRB);
  Value *PtrOp = GEP->getPointerOperand();
  PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());
  IGC_ASSERT_MESSAGE(PtrTy, "Only accept scalar pointer!");
  int IdxWidth = 1;
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI) {
    Value *GEPIdx = *OI;
    if (GEPIdx->getType()->isVectorTy()) {
      auto Width =
          cast<IGCLLVM::FixedVectorType>(GEPIdx->getType())->getNumElements();
      if (Width > 1) {
        if (IdxWidth <= 1)
          IdxWidth = Width;
        else
          IGC_ASSERT_MESSAGE(IdxWidth == Width,
                             "GEP has inconsistent vector-index width");
      }
    }
  }
  Type *Ty = PtrTy;
  auto GTI = gep_type_begin(GEP);
  Value *ScalarizedIdx =
      (IdxWidth == 1)
          ? IRB.getInt32(0)
          : ConstantVector::getSplat(IGCLLVM::getElementCount(IdxWidth),
                                     IRB.getInt32(0));
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *GEPIdx = *OI;
    if (auto *StTy = GTI.getStructTypeOrNull()) {
      auto Field = cast<ConstantInt>(GEPIdx)->getZExtValue();
      if (Field) {
        int Offset = DL->getStructLayout(StTy)->getElementOffset(Field);
        IGC_ASSERT(Idx.getElementSizeInBytes());
        IGC_ASSERT_MESSAGE(
            Offset % Idx.getElementSizeInBytes() == 0,
            "the offset must be a multiple of the current vector granulation");
        Constant *OffsetVal =
            IRB.getInt32(Offset / Idx.getElementSizeInBytes());
        if (IdxWidth > 1)
          OffsetVal = ConstantVector::getSplat(
              IGCLLVM::getElementCount(IdxWidth), OffsetVal);
        ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, OffsetVal);
      }
    } else {
      Ty = GTI.getIndexedType();
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(GEPIdx)) {
        if (!CI->isZero()) {
          Constant *OffsetVal =
              IRB.getInt32(DL->getTypeAllocSize(Ty) * CI->getZExtValue() /
                           Idx.getElementSizeInBytes());
          if (IdxWidth > 1)
            OffsetVal = ConstantVector::getSplat(
                IGCLLVM::getElementCount(IdxWidth), OffsetVal);
          ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, OffsetVal);
        }
      } else if (!GEPIdx->getType()->isVectorTy() && IdxWidth <= 1) {
        Value *NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, IRB.getInt32Ty());
        IGC_ASSERT(Idx.getElementSizeInBytes());
        IGC_ASSERT_MESSAGE(
            DL->getTypeAllocSize(Ty) % Idx.getElementSizeInBytes() == 0,
            "current type size must be multiple of current offset granulation "
            "to be represented in this offset");
        auto ElementSize =
            DL->getTypeAllocSize(Ty) / Idx.getElementSizeInBytes();
        NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
        ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, NewIdx);
      } else {
        // the input idx is a vector or the one of the GEP index is vector
        Value *NewIdx = nullptr;
        IGC_ASSERT_MESSAGE(
            DL->getTypeAllocSize(Ty) % Idx.getElementSizeInBytes() == 0,
            "current type size must be multiple of current offset granulation "
            "to be represented in this offset");
        auto ElementSize =
            DL->getTypeAllocSize(Ty) / Idx.getElementSizeInBytes();
        if (GEPIdx->getType()->isVectorTy()) {
          IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(GEPIdx->getType())
                         ->getNumElements() == IdxWidth);
          NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, ScalarizedIdx->getType());
          NewIdx = IRB.CreateMul(NewIdx, ConstantVector::getSplat(
                                             IGCLLVM::getElementCount(IdxWidth),
                                             IRB.getInt32(ElementSize)));
        } else {
          NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, IRB.getInt32Ty());
          // splat the new-idx into a vector
          NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
        }
        ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, NewIdx);
      }
    }
  }
  if (!Idx.Index->getType()->isVectorTy() && IdxWidth <= 1) {
    ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, Idx.Index);
  } else if (Idx.Index->getType()->isVectorTy()) {
    IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(Idx.Index->getType())
                   ->getNumElements() == IdxWidth);
    ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, Idx.Index);
  } else {
    auto SplatIdx = IRB.CreateVectorSplat(IdxWidth, Idx.Index);
    ScalarizedIdx = IRB.CreateAdd(ScalarizedIdx, SplatIdx);
  }
  handleAllocaSources(*GEP, {ScalarizedIdx, Idx.ElementSizeInBits});
}

// Pass acummulated idx through new phi
void TransposeHelper::handlePHINode(PHINode *Phi, GenericVectorIndex Idx,
                                    BasicBlock *IncomingBB) {
  PHINode *NewPhi = nullptr;
  // If phi is not yet visited
  if (!PhiReplacement.count(Phi)) {
    IRBuilder<> IRB(Phi);
    NewPhi =
        IRB.CreatePHI(Idx.Index->getType(), Phi->getNumIncomingValues(), "idx");
    PhiReplacement.insert(std::make_pair(Phi, NewPhi));
    ToBeRemoved.push_back(Phi);
  } else
    NewPhi = PhiReplacement[Phi];
  NewPhi->addIncoming(Idx.Index, IncomingBB);
  handleAllocaSources(*Phi, {NewPhi, Idx.ElementSizeInBits});
}

void TransposeHelper::handleLoadInst(LoadInst *Load, GenericVectorIndex Idx) {
  IGC_ASSERT(Load->isSimple());
  IRBuilder<> IRB(Load);
  Idx.adjust(Load->getType(), IRB);
  auto *ScalarizedIdx =
      IRB.CreateMul(Idx.Index, ConstantInt::get(Idx.Index->getType(),
                                                Idx.getElementSizeInBytes()));
  auto LdTy = Load->getType()->getScalarType();
  auto *ReadIn = loadAndCastVector(VectorAlloca, LdTy, IRB);
  if (vc::isFunctionPointerType(Load->getType())) {
    Region R(IGCLLVM::FixedVectorType::get(
                 cast<VectorType>(VectorAlloca->getAllocatedType())
                     ->getElementType(),
                 DL->getTypeSizeInBits(LdTy) /
                     DL->getTypeSizeInBits(
                         cast<VectorType>(VectorAlloca->getAllocatedType())
                             ->getElementType())),
             DL);
    if (!ScalarizedIdx->getType()->isIntegerTy(16)) {
      ScalarizedIdx = IRB.CreateZExtOrTrunc(
          ScalarizedIdx, Type::getInt16Ty(Load->getContext()));
    }
    R.Indirect = ScalarizedIdx;
    auto *Result = R.createRdRegion(ReadIn, Load->getName(), Load,
                                    Load->getDebugLoc(), true);
    if (!Result->getType()->isPointerTy()) {
      auto *BC =
          IRB.CreateBitCast(Result, Type::getInt64Ty(Load->getContext()));
      auto *PtrToI = IRB.CreateIntToPtr(BC, Load->getType(), Load->getName());
      Load->replaceAllUsesWith(PtrToI);
    } else
      Load->replaceAllUsesWith(Result);
  } else if (Load->getType()->isVectorTy()) {
    // A vector load
    // %v = load <2 x float>* %ptr
    // becomes
    // %w = load <32 x float>* %ptr1
    // %v0 = extractelement <32 x float> %w, i32 %idx
    // %v1 = extractelement <32 x float> %w, i32 %idx+1
    // replace all uses of %v with <%v0, %v1>
    auto Len =
        cast<IGCLLVM::FixedVectorType>(Load->getType())->getNumElements();
    Value *Result = UndefValue::get(Load->getType());
    for (unsigned I = 0; I < Len; ++I) {
      Value *VectorIdx = ConstantInt::get(Idx.Index->getType(), I);
      auto Index = IRB.CreateAdd(Idx.Index, VectorIdx);
      auto Val = IRB.CreateExtractElement(ReadIn, Index);
      Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
    }
    Load->replaceAllUsesWith(Result);
  } else {
    auto Result = IRB.CreateExtractElement(ReadIn, Idx.Index);
    Load->replaceAllUsesWith(Result);
  }
  Load->eraseFromParent();
}

void TransposeHelper::handleStoreInst(StoreInst *Store,
                                      GenericVectorIndex Idx) {
  // Add Store instruction to remove list
  IGC_ASSERT(Store->isSimple());
  IRBuilder<> IRB(Store);
  Value *StoreVal = Store->getValueOperand();
  Idx.adjust(StoreVal->getType(), IRB);
  auto *ScalarizedIdx =
      IRB.CreateMul(Idx.Index, ConstantInt::get(Idx.Index->getType(),
                                                Idx.getElementSizeInBytes()));
  auto *StTy = StoreVal->getType()->getScalarType();
  Value *WriteOut = loadAndCastVector(VectorAlloca, StTy, IRB);

  if (vc::isFunctionPointerType(StoreVal->getType())) {
    auto *NewStoreVal = StoreVal;
    IGC_ASSERT(cast<VectorType>(VectorAlloca->getAllocatedType())
                   ->getElementType()
                   ->isIntegerTy(64));
    if (vc::isFunctionPointerType(NewStoreVal->getType())) {
      NewStoreVal = IRB.CreatePtrToInt(
          NewStoreVal, IntegerType::getInt64Ty(Store->getContext()));
    }
    Region R(NewStoreVal, DL);
    if (!ScalarizedIdx->getType()->isIntegerTy(16)) {
      ScalarizedIdx = IRB.CreateZExtOrTrunc(
          ScalarizedIdx, Type::getInt16Ty(Store->getContext()));
    }
    if (auto *ConstIdx = dyn_cast<Constant>(ScalarizedIdx))
      R.Indirect = ConstantExpr::getMul(
          ConstIdx,
          ConstantInt::get(
              IRB.getInt16Ty(),
              DL->getTypeSizeInBits(NewStoreVal->getType()->getScalarType()) /
                  genx::ByteBits));
    else
      R.Indirect = ScalarizedIdx;
    WriteOut =
        R.createWrRegion(WriteOut, NewStoreVal, Store->getName() + ".promoted",
                         Store, Store->getDebugLoc());
  } else if (StoreVal->getType()->isVectorTy()) {
    // A vector store
    // store <2 x float> %v, <2 x float>* %ptr
    // becomes
    // %w = load <32 x float> *%ptr1
    // %v0 = extractelement <2 x float> %v, i32 0
    // %w0 = insertelement <32 x float> %w, float %v0, i32 %idx
    // %v1 = extractelement <2 x float> %v, i32 1
    // %w1 = insertelement <32 x float> %w0, float %v1, i32 %idx+1
    // store <32 x float> %w1, <32 x float>* %ptr1
    auto Len =
        cast<IGCLLVM::FixedVectorType>(StoreVal->getType())->getNumElements();
    for (unsigned I = 0; I < Len; ++I) {
      Value *VectorIdx = ConstantInt::get(Idx.Index->getType(), I);
      auto *Val = IRB.CreateExtractElement(StoreVal, VectorIdx);
      auto *Index = IRB.CreateAdd(Idx.Index, VectorIdx);
      IGC_ASSERT_MESSAGE(
          DL->getTypeSizeInBits(Val->getType()) == Idx.ElementSizeInBits,
          "stored type considered vector element size must correspond");
      WriteOut = IRB.CreateInsertElement(WriteOut, Val, Index);
    }
  } else {
    IGC_ASSERT_MESSAGE(
        DL->getTypeSizeInBits(StoreVal->getType()) == Idx.ElementSizeInBits,
        "stored type considered vector element size must correspond");
    WriteOut = IRB.CreateInsertElement(WriteOut, StoreVal, Idx.Index);
  }
  castAndStoreVector(VectorAlloca, WriteOut, IRB);
  Store->eraseFromParent();
}

void TransposeHelper::handleGather(IntrinsicInst *Inst, GenericVectorIndex Idx,
                                   unsigned MaskIndex, unsigned ValueIndex) {
  IRBuilder<> IRB(Inst);
  Idx.adjust(Type::getInt8Ty(Inst->getContext()), IRB);
  auto *ScalarizedIdx =
      IRB.CreateMul(Idx.Index, ConstantInt::get(Idx.Index->getType(),
                                                Idx.getElementSizeInBytes()));
  auto *InstTy = cast<IGCLLVM::FixedVectorType>(Inst->getType());
  auto *ElemTy = InstTy->getElementType();
  auto *LoadVecAlloca = loadAndCastVector(VectorAlloca, ElemTy, IRB);

  auto *IndexOrigVTy = cast<IGCLLVM::FixedVectorType>(ScalarizedIdx->getType());
  auto *IndexVTy = IGCLLVM::FixedVectorType::get(
      IRB.getInt16Ty(), IndexOrigVTy->getNumElements());

  Region R(Inst);
  R.Indirect = IRB.CreateTrunc(ScalarizedIdx, IndexVTy);
  R.Width = 1;
  R.Stride = 0;
  R.VStride = 0;
  Value *Result =
      R.createRdRegion(LoadVecAlloca, Inst->getName(), Inst /*InsertBefore*/,
                       Inst->getDebugLoc(), true /*AllowScalar*/);

  // if old-value is not undefined and predicate is not all-one, create a select
  auto *PredVal = Inst->getArgOperand(MaskIndex);
  bool PredAllOne = false;
  if (auto *C = dyn_cast<ConstantVector>(PredVal)) {
    auto *Splat = C->getSplatValue();
    PredAllOne = Splat && Splat->isOneValue();
  }

  auto *OldVal = Inst->getArgOperand(ValueIndex);
  if (!PredAllOne && !isa<UndefValue>(OldVal))
    Result = IRB.CreateSelect(PredVal, Result, OldVal);

  Inst->replaceAllUsesWith(Result);
  Inst->eraseFromParent();
}

void TransposeHelper::handleScatter(IntrinsicInst *Inst, GenericVectorIndex Idx,
                                    unsigned MaskIndex, unsigned ValueIndex) {
  IRBuilder<> IRB(Inst);
  Idx.adjust(Type::getInt8Ty(Inst->getContext()), IRB);
  auto *StoreVal = Inst->getArgOperand(ValueIndex);
  auto *ScalarizedIdx =
      IRB.CreateMul(Idx.Index, ConstantInt::get(Idx.Index->getType(),
                                                Idx.getElementSizeInBytes()));
  auto *StoreTy = cast<IGCLLVM::FixedVectorType>(StoreVal->getType());
  auto *ElemTy = StoreTy->getElementType();
  auto *LoadVecAlloca = loadAndCastVector(VectorAlloca, ElemTy, IRB);

  auto *IndexOrigVTy = cast<IGCLLVM::FixedVectorType>(ScalarizedIdx->getType());
  auto *IndexVTy = IGCLLVM::FixedVectorType::get(
      IRB.getInt16Ty(), IndexOrigVTy->getNumElements());

  Region R(StoreVal);
  R.Mask = Inst->getArgOperand(MaskIndex);
  R.Indirect = IRB.CreateTrunc(ScalarizedIdx, IndexVTy);
  R.Width = 1;
  R.Stride = 0;
  R.VStride = 0;
  auto *NewInst = R.createWrRegion(LoadVecAlloca, StoreVal, Inst->getName(),
                                   Inst, Inst->getDebugLoc());
  castAndStoreVector(VectorAlloca, NewInst, IRB);
  Inst->eraseFromParent();
}

void TransposeHelper::handleLifetimeStart(IntrinsicInst *II,
                                          GenericVectorIndex Idx) {
  auto IID = vc::getAnyIntrinsicID(II);
  IGC_ASSERT_EXIT(IID == Intrinsic::lifetime_start);

  IRBuilder<> IRB(II);
  IGC_ASSERT_EXIT(Idx.Index == IRB.getInt32(0));

  auto *Ty = VectorAlloca->getAllocatedType();
  auto *SizeC = IRB.getInt64(DL->getTypeSizeInBits(Ty) / ByteBits);

  IRB.CreateLifetimeStart(VectorAlloca, SizeC);

  // The promotion pass generates load instruction even if the alloca memory is
  // not initialized. So mem2reg transformation emits unnecessary PHI-nodes.
  // Adding undef store avoids such PHIs.
  IRB.CreateStore(UndefValue::get(Ty), VectorAlloca);

  II->eraseFromParent();
}

void TransposeHelper::handleLifetimeEnd(IntrinsicInst *II,
                                        GenericVectorIndex Idx) {
  auto IID = vc::getAnyIntrinsicID(II);
  IGC_ASSERT_EXIT(IID == Intrinsic::lifetime_end);

  IRBuilder<> IRB(II);
  IGC_ASSERT_EXIT(Idx.Index == IRB.getInt32(0));

  auto *Ty = VectorAlloca->getAllocatedType();
  auto *SizeC = IRB.getInt64(DL->getTypeSizeInBits(Ty) / ByteBits);

  IRB.CreateLifetimeEnd(VectorAlloca, SizeC);

  II->eraseFromParent();
}

bool GenXPromoteArray::runOnFunction(Function &F) {
  Func = &F;
  Ctx = &(Func->getContext());

  DL = &F.getParent()->getDataLayout();
  ForcePromotion = getAnalysis<GenXBackendConfig>().isArrayPromotionForced() &&
                   TotalAllocaLimitOpt.getNumOccurrences() == 0 &&
                   SingleAllocaLimitOpt.getNumOccurrences() == 0;
  LargeAllocasWereLeft = false;
  AllocasToPrivMem.clear();

  visit(F);

  unsigned AggregatesReplaced = 0;
  while (!AggregatesToReplace.empty()) {
    auto *I = AggregatesToReplace.pop_back_val();
    if (auto SI = dyn_cast<StoreInst>(I))
      replaceAggregatedStore(SI);
    else
      replaceAggregatedLoad(cast<LoadInst>(I));
    AggregatesReplaced++;
  }

  selectAllocasToHandle();

  if (LargeAllocasWereLeft)
    vc::warn(vc::WarningName::Generic, F.getContext(), *this,
             F.getName() + " allocation size is too big for promotion, using "
                           "stack allocation");

  for (auto *Alloca : AllocasToPrivMem)
    handleAllocaInst(Alloca);

  // Last remove alloca instructions
  for (auto *Inst : AllocasToPrivMem)
    if (Inst->use_empty())
      Inst->eraseFromParent();

  // IR changed only if we had alloca instruction to optimize or if aggregated
  // stores were replaced
  return !AllocasToPrivMem.empty() || AggregatesReplaced > 0;
}

IGCLLVM::FixedVectorType &
GenXPromoteArray::getVectorTypeForAlloca(AllocaInst &Alloca,
                                         Type &ElemTy) const {
  auto AllocaSize = IGCLLVM::makeOptional(Alloca.getAllocationSizeInBits(*DL));
  IGC_ASSERT_MESSAGE(AllocaSize.has_value(), "VLA is not expected");
  auto NumElem = AllocaSize.value() / DL->getTypeAllocSizeInBits(&ElemTy);
  return *IGCLLVM::FixedVectorType::get(&ElemTy, NumElem);
}

AllocaInst *GenXPromoteArray::createVectorForAlloca(AllocaInst *Alloca,
                                                    Type *BaseTy) {
  IRBuilder<> IRB(Alloca);
  auto &VecType = getVectorTypeForAlloca(*Alloca, *BaseTy);
  return IRB.CreateAlloca(&VecType);
}

void GenXPromoteArray::replaceAggregatedStore(StoreInst *SI) {
  IGC_ASSERT(SI->isSimple());
  IRBuilder<> Builder(SI);
  auto *Ptr = SI->getPointerOperand();
  auto *Val = SI->getValueOperand();
  auto *ValTy = Val->getType();
  auto *ArrTy = dyn_cast<ArrayType>(ValTy);
  uint64_t NumElements =
      ArrTy ? ArrTy->getNumElements() : ValTy->getNumContainedTypes();
  auto *IdxType = ArrTy ? Type::getInt64Ty(*Ctx) : Type::getInt32Ty(*Ctx);
  for (uint64_t Idx = 0; Idx < NumElements; Idx++) {
    auto *ElemVal = Builder.CreateExtractValue(Val, Idx);
    Value *Indices[2] = {ConstantInt::get(IdxType, 0),
                         ConstantInt::get(IdxType, Idx)};
    auto *ElemPtr = Builder.CreateInBoundsGEP(Val->getType(), Ptr,
                                              MutableArrayRef(Indices));
    auto *ElemSI = Builder.CreateStore(ElemVal, ElemPtr);
    if (ElemVal->getType()->isAggregateType())
      AggregatesToReplace.push_back(ElemSI);
  }
  SI->eraseFromParent();
}

void GenXPromoteArray::replaceAggregatedLoad(LoadInst *LI) {
  IGC_ASSERT(LI->isSimple());
  IRBuilder<> Builder(LI);
  auto *Ptr = LI->getPointerOperand();
  auto *Ty = LI->getType();
  auto *ArrTy = dyn_cast<ArrayType>(Ty);
  uint64_t NumElements =
      ArrTy ? ArrTy->getNumElements() : Ty->getNumContainedTypes();
  auto *IdxType = ArrTy ? Type::getInt64Ty(*Ctx) : Type::getInt32Ty(*Ctx);
  auto *Result = cast<Value>(UndefValue::get(Ty));
  for (uint64_t Idx = 0; Idx < NumElements; Idx++) {
    auto *ElemTy = ArrTy ? ArrTy->getElementType() : Ty->getContainedType(Idx);
    Value *Indices[2] = {ConstantInt::get(IdxType, 0),
                         ConstantInt::get(IdxType, Idx)};
    auto *ElemPtr =
        Builder.CreateInBoundsGEP(Ty, Ptr, MutableArrayRef(Indices));
    auto *ElemLI = Builder.CreateLoad(ElemTy, ElemPtr);
    if (ElemTy->isAggregateType())
      AggregatesToReplace.push_back(ElemLI);
    Result = Builder.CreateInsertValue(Result, ElemLI, Idx);
  }
  auto Name = LI->getName();
  Result->setName(Name);
  LI->replaceAllUsesWith(Result);
  LI->eraseFromParent();
}

unsigned int GenXPromoteArray::extractAllocaSize(AllocaInst *Alloca) {
  unsigned int ArraySize =
      cast<ConstantInt>(Alloca->getArraySize())->getZExtValue();
  unsigned int totalArrayStructureSize =
      DL->getTypeAllocSize(Alloca->getAllocatedType()) * ArraySize;

  return totalArrayStructureSize;
}

bool GenXPromoteArray::checkPtrToIntCandidate(PtrToIntInst *PTI,
                                              Type *CurBaseTy) const {
  // Here we handle only the most common patterns for LLVM and SVM
  // gather/scatter instructions:
  //   * ptrtoint->insertelem->shuffle->arith_op->svm_gather/scatter,
  //   * ptrtoint->insertelem->shuffle->arith_op->inttoptr->gather/scatter.
  // Others are possible, but not handled yet
  if (!PTI->hasOneUse())
    return false;
  auto *Insert = dyn_cast<InsertElementInst>(PTI->user_back());
  if (!Insert)
    return false;
  if (!Insert->hasOneUse())
    return false;
  auto *Shuffle = dyn_cast<ShuffleVectorInst>(Insert->user_back());
  if (!Shuffle)
    return false;
  if (!Shuffle->hasOneUse())
    return false;
  auto *BinOp = dyn_cast<BinaryOperator>(Shuffle->user_back());
  if (!BinOp)
    return false;
  if (BinOp->user_empty())
    return false;

  Value *Address = BinOp;
  if (BinOp->hasOneUse() && isa<IntToPtrInst>(BinOp->user_back())) {
    Address = BinOp->user_back();
    if (Address->user_empty())
      return false;
  }

  for (auto *MemOp : Address->users()) {
    Value *Pred = nullptr;
    Value *Input = nullptr;
    unsigned NumBlocks = 0;
    switch (vc::getAnyIntrinsicID(MemOp)) {
    default:
      return false;
    case Intrinsic::masked_gather:
      Pred = MemOp->getOperand(2);
      Input = MemOp->getOperand(3);
      break;
    case Intrinsic::masked_scatter:
      Pred = MemOp->getOperand(3);
      Input = MemOp->getOperand(0);
      break;
    case GenXIntrinsic::genx_svm_gather:
    case GenXIntrinsic::genx_svm_scatter: {
      Pred = MemOp->getOperand(0);
      Input = MemOp->getOperand(3);

      auto *NumBlocksV = MemOp->getOperand(1);
      IGC_ASSERT(isa<ConstantInt>(NumBlocksV));
      NumBlocks = cast<ConstantInt>(NumBlocksV)->getZExtValue();
    } break;
    }
    // For now skip insts w/ blockSize > 1 or weird things like
    // <16 x i32> %res = svm.gather(<8 x i64> offsets, ...)
    // Ignore reads of types different from alloca types, e.g.
    // %v0 = alloca [16 x i8]
    // .. store of global to %v0
    // %offsets = %v0 + <0, 4, 8, 12, 0, 4, 8, 12>
    // ....
    // %v1 = <8 x float> svm_gather %v0, %offsets, <8 x float> undef
    // OR
    // svm_scatter %v0, %offset, <8 x float> %value
    if (Input->getType()->getScalarType() != CurBaseTy)
      return false;

    if (NumBlocks != 0)
      return false;
    if (cast<IGCLLVM::FixedVectorType>(Input->getType())->getNumElements() >
        cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements())
      return false;
    if (isa<VectorType>(MemOp->getType()) &&
        cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements() <
            cast<IGCLLVM::FixedVectorType>(MemOp->getType())->getNumElements())
      return false;
  }
  return true;
}

bool GenXPromoteArray::checkTypes(Type *CurBaseTy, Type *NewTy) const {
  auto *NewBaseTy = getBaseType(NewTy, nullptr);
  // either the point-to-element-type is the same or
  // the point-to-element-type is the byte or a function pointer
  if (!CurBaseTy || !NewBaseTy)
    return false;
  if (NewBaseTy->getScalarSizeInBits() != 8 &&
      CurBaseTy->getScalarSizeInBits() != NewBaseTy->getScalarSizeInBits() &&
      !vc::isFunctionPointerType(NewBaseTy))
    return false;
  return true;
}

bool GenXPromoteArray::checkAllocaUsesInternal(Instruction *I, Type *CurBaseTy,
                                               bool NeedCheckTypes) const {
  for (Value::user_iterator UseIt = I->user_begin(), UseE = I->user_end();
       UseIt != UseE; ++UseIt) {
    if (auto *GEP = dyn_cast<GetElementPtrInst>(*UseIt)) {
      if (NeedCheckTypes && !checkTypes(CurBaseTy, GEP->getSourceElementType()))
        return false;
      auto *PtrV = GEP->getPointerOperand();
      // we cannot support a vector of pointers as the base of the GEP
      if (!PtrV->getType()->isPointerTy() ||
          !checkAllocaUsesInternal(
              GEP, getBaseType(GEP->getResultElementType(), nullptr), false))
        return false;
    } else if (auto *Load = dyn_cast<LoadInst>(*UseIt)) {
      if (NeedCheckTypes && !checkTypes(CurBaseTy, Load->getType()))
        return false;
      if (!Load->isSimple())
        return false;
    } else if (auto *Store = dyn_cast<StoreInst>(*UseIt)) {
      if (NeedCheckTypes &&
          !checkTypes(CurBaseTy, Store->getValueOperand()->getType()))
        return false;
      if (!Store->isSimple())
        return false;
      // GEP instruction is the stored value of the StoreInst (not supported)
      if (Store->getValueOperand() == I)
        return false;
    } else if (auto *Cast = dyn_cast<BitCastInst>(*UseIt)) {
      if (Cast->use_empty())
        continue;
      if (!checkAllocaUsesInternal(Cast, CurBaseTy, true))
        return false;
    } else if (auto *PTI = dyn_cast<PtrToIntInst>(*UseIt)) {
      if (!checkPtrToIntCandidate(PTI, CurBaseTy))
        return false;
    } else if (auto *II = dyn_cast<IntrinsicInst>(*UseIt)) {
      auto IID = vc::getAnyIntrinsicID(II);
      switch (IID) {
      case Intrinsic::lifetime_start:
      case Intrinsic::lifetime_end:
        break;
      case Intrinsic::masked_gather:
        if (NeedCheckTypes && !checkTypes(CurBaseTy, II->getType()))
          return false;
        break;
      case Intrinsic::masked_scatter:
        if (NeedCheckTypes &&
            !checkTypes(CurBaseTy, II->getOperand(0)->getType()))
          return false;
        break;
      default:
        return false;
      }
    } else if (auto *Phi = dyn_cast<PHINode>(*UseIt)) {
      // Only GEPs with same base and bitcasts with same src yet supported
      Value *PtrOp = nullptr;
      if (auto *BC = dyn_cast<BitCastInst>(I))
        PtrOp = BC->getOperand(0);
      else if (auto *GEP = dyn_cast<GetElementPtrInst>(I))
        PtrOp = GEP->getPointerOperand();
      else
        return false;

      auto IsValid = [&](Value *V) {
        if (auto *GEP = dyn_cast<GetElementPtrInst>(V))
          return GEP->getPointerOperand() == PtrOp;
        else if (auto *BC = dyn_cast<BitCastInst>(V))
          return BC->getOperand(0) == PtrOp;
        return false;
      };
      if (!all_of(Phi->incoming_values(), IsValid) ||
          !checkAllocaUsesInternal(Phi, CurBaseTy, NeedCheckTypes))
        return false;
    } else {
      // This is some other instruction. Right now we don't want to handle these
      return false;
    }
  }
  return true;
}

bool GenXPromoteArray::isAllocaPromotable(AllocaInst &Alloca) {
  // Cannot promote VLA.
  auto MaybeSize = IGCLLVM::makeOptional(Alloca.getAllocationSizeInBits(*DL));

  if (!MaybeSize.has_value())
    return false;

  auto AllocaSize = MaybeSize.value() / genx::ByteBits;
  if (!ForcePromotion && AllocaSize > SingleAllocaLimitOpt.getValue()) {
    LargeAllocasWereLeft = true;
    return false;
  }

  // Don't even look at non-array or non-struct allocas.
  // (extractAllocaDim can not handle them anyway, causing a crash)
  auto *Ty = Alloca.getAllocatedType();
  if ((!Ty->isStructTy() && !Ty->isArrayTy() && !Ty->isVectorTy()) ||
      Alloca.isArrayAllocation())
    return false;

  auto *BaseTy = getBaseType(Ty, nullptr);
  if (BaseTy == nullptr)
    return false;
  auto *ScalarTy = BaseTy->getScalarType();
  // only handle case with a simple base type
  if (!(ScalarTy->isFloatingPointTy() || ScalarTy->isIntegerTy()) &&
      !vc::isFunctionPointerType(ScalarTy))
    return false;

  // After promotion the variable will be illegal.
  auto &VecTy = getVectorTypeForAlloca(Alloca, *ScalarTy);
  if (!visa::Variable::isLegal(VecTy, *DL))
    return false;

  return checkAllocaUsesInternal(&Alloca, BaseTy, false);
}

void GenXPromoteArray::visitStore(StoreInst &I) {
  if (I.getValueOperand()->getType()->isAggregateType())
    AggregatesToReplace.push_back(&I);
}

void GenXPromoteArray::visitLoad(LoadInst &I) {
  if (I.getType()->isAggregateType())
    AggregatesToReplace.push_back(&I);
}

void GenXPromoteArray::visitAllocaInst(AllocaInst &I) {
  // find those allocas that can be promoted as a whole-vector
  if (isAllocaPromotable(I))
    AllocasToPrivMem.push_back(&I);
}

void GenXPromoteArray::selectAllocasToHandle() {
  if (AllocasToPrivMem.empty())
    return;
  // Promote them all.
  if (ForcePromotion)
    return;

  std::sort(AllocasToPrivMem.begin(), AllocasToPrivMem.end(),
            [this](const AllocaInst *LHS, const AllocaInst *RHS) {
              auto LHSSize =
                  IGCLLVM::makeOptional(LHS->getAllocationSizeInBits(*DL));
              auto RHSSize =
                  IGCLLVM::makeOptional(RHS->getAllocationSizeInBits(*DL));

              return LHSSize.value() < RHSSize.value();
            });
  auto LastIt = vc::upper_partial_sum_bound(
      AllocasToPrivMem.begin(), AllocasToPrivMem.end(),
      TotalAllocaLimitOpt.getValue(),
      [this](std::size_t PrevSum, const AllocaInst *CurAlloca) {
        auto CurAllocaSize =
            IGCLLVM::makeOptional(CurAlloca->getAllocationSizeInBits(*DL));
        return PrevSum + CurAllocaSize.value() / genx::ByteBits;
      });

  // if alloca size exceeds alloc size threshold, emit warning
  // and discard promotion
  if (LastIt != AllocasToPrivMem.end())
    LargeAllocasWereLeft = true;
  AllocasToPrivMem.erase(LastIt, AllocasToPrivMem.end());
}

void GenXPromoteArray::handleAllocaInst(AllocaInst *Alloca) {
  // Extract the Alloca size and the base Type
  auto *BaseTy = getBaseType(Alloca->getAllocatedType(), nullptr);
  if (!BaseTy)
    return;
  BaseTy = BaseTy->getScalarType();
  auto *VecAlloca = createVectorForAlloca(Alloca, BaseTy);
  if (!VecAlloca)
    return;
  // skip processing of allocas that are already fine
  if (VecAlloca->getAllocatedType() == Alloca->getAllocatedType())
    return;

  IRBuilder<> IRB(VecAlloca);
  GenericVectorIndex StartIdx{
      IRB.getInt32(0), static_cast<unsigned>(DL->getTypeSizeInBits(BaseTy))};
  TransposeHelper Helper(VecAlloca, DL);
  Helper.handleAllocaSources(*Alloca, StartIdx);
  Helper.EraseDeadCode();
}
} // namespace
