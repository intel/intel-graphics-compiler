/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
/// This pass lowers alloca instructions to genx.alloca intrinsics and changes
/// pointer from alloca to offset in predefined stack surface
//
//===----------------------------------------------------------------------===//

#include "GenX.h"
#include "GenXModule.h"
#include "GenXRegion.h"
#include "GenXSubtarget.h"
#include "GenXUtil.h"
#include "GenXVisa.h"
#include "llvmWrapper/IR/InstrTypes.h"
#include "llvm/ADT/SmallVector.h"
#include "llvm/GenXIntrinsics/GenXMetadata.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"
#include <queue>
#include <utility>

using namespace llvm;
using namespace genx;

#define DEBUG_TYPE "genx-tpm"

namespace {

// This actually should've been a FunctionGroupPass,
// but due to the FGPassManager hack we can't run GenXModule twice
// so for now we can't insert module pass that invalidate FGA betw FGPasses
class GenXThreadPrivateMemory : public ModulePass,
                                public InstVisitor<GenXThreadPrivateMemory> {
public:
  GenXThreadPrivateMemory();

  virtual StringRef getPassName() const override {
    return "GenXThreadPrivateMemory";
  }

  void getAnalysisUsage(AnalysisUsage &AU) const {
    ModulePass::getAnalysisUsage(AU);
    AU.setPreservesCFG();
  }

  bool runOnModule(Module &M) override;
  bool runOnFunction(Function &F);

  void visitAllocaInst(AllocaInst &I);
  void visitFunction(Function &F);

private:
  bool replacePhi(PHINode *Phi);
  bool preparePhiForReplacement(PHINode *Phi);
  bool replaceScatterPrivate(CallInst *CI);
  bool replaceGatherPrivate(CallInst *CI);
  bool replacePTI(PtrToIntInst *PTI);
  bool replaceStore(StoreInst *StI);
  bool replaceLoad(LoadInst *LdI);
  bool replaceSelect(SelectInst *Sel);
  bool replaceAddrSpaceCast(AddrSpaceCastInst * AddrCast);
  Value *lookForPtrReplacement(Value *Ptr) const;
  void addUsers(Value *V);
  void collectEachPossibleTPMUsers();
  void addUsersIfNeeded(Value *V);
  std::pair<Value *, unsigned> NormalizeVector(Value *From, Type *To,
                                               Instruction *InsertBefore);
  Instruction *RestoreVectorAfterNormalization(Instruction *From, Type *To);

public:
  static char ID;

private:
  LLVMContext *m_ctx;
  const GenXSubtarget *m_ST;
  const DataLayout *m_DL;
  std::vector<AllocaInst *> m_alloca;
  std::vector<Argument *> m_args;
  std::vector<CallInst *> m_gather;
  std::vector<CallInst *> m_scatter;
  std::map<AllocaInst *, CallInst *> m_allocaToIntrinsic;
  std::queue<Instruction *> m_AIUsers;
  std::set<Instruction *> m_AlreadyAdded;
  PreDefined_Surface m_stack;
  bool m_useGlobalMem = false;
};
} // namespace

// Register pass to igc-opt
namespace llvm {
void initializeGenXThreadPrivateMemoryPass(PassRegistry &);
}

INITIALIZE_PASS_BEGIN(GenXThreadPrivateMemory, "GenXThreadPrivateMemory",
                      "GenXThreadPrivateMemory", false, false)
INITIALIZE_PASS_END(GenXThreadPrivateMemory, "GenXThreadPrivateMemory",
                    "GenXThreadPrivateMemory", false, false)

char GenXThreadPrivateMemory::ID = 0;

ModulePass *llvm::createGenXThreadPrivateMemoryPass() {
  return new GenXThreadPrivateMemory;
}

GenXThreadPrivateMemory::GenXThreadPrivateMemory() : ModulePass(ID) {
  initializeGenXThreadPrivateMemoryPass(*PassRegistry::getPassRegistry());
}

static Value *ZExtOrTruncIfNeeded(Value *From, Type *To,
                                  Instruction *InsertBefore) {
  unsigned FromTySz = From->getType()->getPrimitiveSizeInBits();
  unsigned ToTySz = To->getPrimitiveSizeInBits();
  Value *Res = From;
  if (From->getType()->isVectorTy() &&
      From->getType()->getVectorNumElements() == 1) {
    auto *TmpRes = CastInst::CreateBitOrPointerCast(
        Res, From->getType()->getVectorElementType(), "", InsertBefore);
    Res = TmpRes;
  }
  if (FromTySz < ToTySz)
    Res = CastInst::CreateZExtOrBitCast(Res, To, "", InsertBefore);
  else if (FromTySz > ToTySz)
    Res = CastInst::CreateTruncOrBitCast(Res, To, "", InsertBefore);
  return Res;
}

// If data is a vector of double/int64, bitcast each element to 2 int32.
// If data is a vector of type < 32bit, extend each element in order to create
// proper send instruction in the finalizer.
std::pair<Value *, unsigned>
GenXThreadPrivateMemory::NormalizeVector(Value *From, Type *To,
                                         Instruction *InsertBefore) {
  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  Value *Res = From;
  Type *FromTy = From->getType();
  assert(isa<VectorType>(FromTy));
  unsigned NumElts = FromTy->getVectorNumElements();
  unsigned EltSz =
      m_DL->getTypeSizeInBits(FromTy->getScalarType()) / genx::ByteBits;
  assert(EltSz > 0);
  if (To->getScalarType()->isPointerTy() &&
      To->getScalarType()->getPointerElementType()->isFunctionTy()) {
    Type *I64Ty = Type::getInt64Ty(InsertBefore->getContext());
    To = VectorType::get(I64Ty, NumElts);
    Res = CastInst::Create(Instruction::PtrToInt, From, To, "", InsertBefore);
    NumElts *= 2;
    To = VectorType::get(I32Ty, NumElts);
    EltSz = I32Ty->getPrimitiveSizeInBits() / genx::ByteBits;
    Res = CastInst::Create(Instruction::BitCast, Res, To, "", InsertBefore);
  } else if (To->getVectorElementType()->getPrimitiveSizeInBits() <
             genx::DWordBits) {
    To = VectorType::get(I32Ty, NumElts);

    Res = CastInst::Create(Instruction::ZExt, From, To, "", InsertBefore);
  } else if (To->getVectorElementType()->getPrimitiveSizeInBits() ==
             genx::QWordBits) {
    NumElts *= 2;
    EltSz = I32Ty->getPrimitiveSizeInBits() / genx::ByteBits;
    To = VectorType::get(I32Ty, NumElts);

    Res = CastInst::Create(Instruction::BitCast, From, To, "", InsertBefore);
  }

  return std::make_pair(Res, EltSz);
}

Instruction *
GenXThreadPrivateMemory::RestoreVectorAfterNormalization(Instruction *From,
                                                         Type *To) {
  Instruction *Restored = From;
  unsigned EltSz = m_DL->getTypeSizeInBits(To->getScalarType());
  assert(EltSz > 0);
  if (To->getScalarType()->isPointerTy() &&
      To->getScalarType()->getPointerElementType()->isFunctionTy()) {
    Restored = PtrToIntInst::Create(Instruction::IntToPtr, From, To);
  } else if (EltSz < genx::DWordBits) {
    Restored = CastInst::Create(Instruction::Trunc, From, To, "");
  } else if (EltSz == genx::QWordBits &&
             !(m_useGlobalMem && To->getScalarType()->isIntegerTy(64))) {
    if (!From->getType()->getScalarType()->isPointerTy() &&
        To->getScalarType()->isPointerTy()) {
      assert(From->getType()->getScalarType()->isIntegerTy(genx::DWordBits));
      Type *NewTy =
          VectorType::get(Type::getInt64Ty(*m_ctx),
                          From->getType()->getVectorNumElements() / 2);
      auto *NewFrom = CastInst::CreateBitOrPointerCast(From, NewTy);
      NewFrom->insertAfter(From);
      From = NewFrom;
      Restored = CastInst::Create(CastInst::IntToPtr, From, To);
    } else
      Restored = CastInst::CreateBitOrPointerCast(From, To);
  }
  if (Restored != From)
    Restored->insertAfter(From);
  return Restored;
}

static Value *DoubleVector(Value *OrigVector, unsigned ShiftVal,
                           Instruction *InsertPoint) {
  IRBuilder<> Builder(InsertPoint);
  Type *I32Ty = Type::getInt32Ty(InsertPoint->getContext());
  unsigned NumElts = OrigVector->getType()->getVectorNumElements() * 2;
  Type *OrigVectorEltTy = OrigVector->getType()->getVectorElementType();
  Value *NewElts = UndefValue::get(VectorType::get(OrigVectorEltTy, NumElts));
  for (unsigned CurEltNum = 0; CurEltNum * 2 < NumElts; ++CurEltNum) {
    Value *OldIdx = ConstantInt::get(I32Ty, CurEltNum);
    Value *NewIdx = ConstantInt::get(I32Ty, CurEltNum * 2);
    Value *EltOld = Builder.CreateExtractElement(OrigVector, OldIdx);
    NewElts = Builder.CreateInsertElement(NewElts, EltOld, NewIdx);
    NewIdx = ConstantInt::get(I32Ty, CurEltNum * 2 + 1);
    if (ShiftVal) {
      Value *TyShift = ConstantInt::get(I32Ty, ShiftVal);
      EltOld = Builder.CreateAdd(EltOld, TyShift);
    }
    NewElts = Builder.CreateInsertElement(NewElts, EltOld, NewIdx);
  }

  return NewElts;
}

static Value *FormEltsOffsetVector(unsigned NumElts, unsigned TySz,
                                   Instruction *InsertBefore) {
  IRBuilder<> Builder(InsertBefore);
  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  Value *EltsOffset = UndefValue::get(VectorType::get(I32Ty, NumElts));
  for (unsigned CurElt = 0; CurElt < NumElts; ++CurElt) {
    Value *Idx = ConstantInt::get(I32Ty, CurElt);
    Value *EltOffset = ConstantInt::get(I32Ty, CurElt * TySz);
    EltsOffset = Builder.CreateInsertElement(EltsOffset, EltOffset, Idx);
  }

  return EltsOffset;
}

static Value *FormEltsOffsetVectorForSVM(unsigned NumElts,
                                         Instruction *InsertBefore,
                                         Value *Offset,
                                         Type *OffsetTy = nullptr) {
  IRBuilder<> Builder(InsertBefore);
  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  if (!OffsetTy)
    OffsetTy = Type::getInt64Ty(InsertBefore->getContext());
  Value *EltsOffset = UndefValue::get(VectorType::get(OffsetTy, NumElts));
  if (Offset->getType()->isVectorTy()) {
    assert(Offset->getType()->getVectorNumElements() == 1);
    Offset = CastInst::CreateZExtOrBitCast(Offset, OffsetTy, "", InsertBefore);
  }
  for (unsigned CurElt = 0; CurElt < NumElts; ++CurElt) {
    Value *Idx = ConstantInt::get(I32Ty, CurElt);
    EltsOffset = Builder.CreateInsertElement(EltsOffset, Offset, Idx);
  }

  return EltsOffset;
}

Value *GenXThreadPrivateMemory::lookForPtrReplacement(Value *Ptr) const {
  assert(Ptr->getType()->isPtrOrPtrVectorTy());

  if (auto BC = dyn_cast<BitCastInst>(Ptr))
    return lookForPtrReplacement(BC->getOperand(0));
  else if (auto ITP = dyn_cast<IntToPtrInst>(Ptr))
    return ITP->getOperand(0);
  else if (auto AI = dyn_cast<AllocaInst>(Ptr)) {
    auto AllocaIntr = m_allocaToIntrinsic.find(AI);
    assert(AllocaIntr != m_allocaToIntrinsic.end() &&
           "Each alloca must be here");
    return AllocaIntr->second;
  } else if (isa<Argument>(Ptr)) {
    if (Ptr->getType()->isPointerTy()) {
      auto *PTI =
          CastInst::Create(CastInst::PtrToInt, Ptr, Type::getInt64Ty(*m_ctx));
      PTI->insertBefore(&cast<Argument>(Ptr)->getParent()->front().front());
      return PTI;
    } else
      return Ptr;
  } else if (isa<ExtractElementInst>(Ptr) &&
             lookForPtrReplacement(
                 cast<ExtractElementInst>(Ptr)->getVectorOperand())) {
    if (Ptr->getType()->isPointerTy()) {
      auto *PTI = CastInst::Create(Instruction::PtrToInt, Ptr,
                              Type::getInt32Ty(*m_ctx));
      PTI->insertAfter(cast<Instruction>(Ptr));
      return PTI;
    } else
      return Ptr;
  } else if (auto *CI = dyn_cast<IGCLLVM::CallInst>(Ptr)) {
    if (!CI->isIndirectCall() &&
        GenXIntrinsic::getAnyIntrinsicID(CI->getCalledFunction()) ==
            GenXIntrinsic::genx_svm_block_ld) {
      return Ptr;
    } else {
      // FIXME: unify the return paths for failure cases
      assert(0 && "Cannot find pointer replacement");
      return nullptr;
    }
  } else if (isa<ConstantPointerNull>(Ptr))
    return ConstantInt::get(Type::getInt32Ty(*m_ctx), 0);
  else {
    assert(0 && "Cannot find pointer replacement");
    return nullptr;
  }
}

bool GenXThreadPrivateMemory::replaceAddrSpaceCast(
  AddrSpaceCastInst* AddrCast) {
  auto NewAlloca = lookForPtrReplacement(AddrCast->getPointerOperand());

  auto IntToPtr = IntToPtrInst::Create(
    llvm::Instruction::CastOps::IntToPtr, NewAlloca,
    AddrCast->getPointerOperand()->getType(), "", AddrCast);
  auto NewAddrCast =
    AddrSpaceCastInst::Create(llvm::Instruction::CastOps::AddrSpaceCast,
    IntToPtr, AddrCast->getType(), "", AddrCast);

  AddrCast->replaceAllUsesWith(NewAddrCast);
  AddrCast->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceLoad(LoadInst *LdI) {
  LLVM_DEBUG(dbgs() << "Replacing load " << *LdI << " ===>\n");
  IRBuilder<> Builder(LdI);
  Type *LdTy = LdI->getType();
  Type *LdEltTy = LdTy;
  if (isa<VectorType>(LdEltTy))
    LdEltTy = LdEltTy->getVectorElementType();
  else
    LdTy = VectorType::get(LdTy, 1);

  unsigned NumEltsToLoad = LdTy->getVectorNumElements();
  unsigned LdEltTySz = m_DL->getTypeSizeInBits(LdEltTy);
  if (!(m_useGlobalMem && LdEltTy->isIntegerTy(64)) &&
      LdEltTySz == genx::QWordBits)
    NumEltsToLoad *= 2;

  Value *PredVal = ConstantInt::get(Type::getInt1Ty(*m_ctx), 1);
  Value *Pred = Builder.CreateVectorSplat(NumEltsToLoad, PredVal);

  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Type *TyToLoad = (m_useGlobalMem && LdEltTy->isIntegerTy(64)) ? I64Ty : I32Ty;
  if (LdEltTy->isFloatTy())
    TyToLoad = LdEltTy;
  Type *RealTyToLoad = LdEltTy;
  if (!(m_useGlobalMem && LdEltTy->isIntegerTy(64)) &&
      m_DL->getTypeSizeInBits(RealTyToLoad) == genx::QWordBits)
    RealTyToLoad = I32Ty;
  unsigned RealTyToLoadSz =
      m_DL->getTypeSizeInBits(RealTyToLoad) / genx::ByteBits;
  Value *OldValOfTheDataRead =
      Builder.CreateVectorSplat(NumEltsToLoad, UndefValue::get(TyToLoad));


  Value *PointerOp = LdI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);
  Offset =
      ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, LdI);
  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_gather
                 : llvm::GenXIntrinsic::genx_gather_scaled;

  Value *EltsOffset = FormEltsOffsetVector(NumEltsToLoad, RealTyToLoadSz, LdI);

  unsigned SrcSize = genx::log2(RealTyToLoadSz);
  Value *logNumBlocks = ConstantInt::get(I32Ty, m_useGlobalMem ? 0 : SrcSize);
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  if (m_useGlobalMem && NumEltsToLoad > 1) {
    assert(Offset->getType()->getScalarType()->isIntegerTy(64));
    auto *BaseOff = FormEltsOffsetVectorForSVM(NumEltsToLoad, LdI, Offset);
    auto *ZextOff = CastInst::CreateZExtOrBitCast(
        EltsOffset,
        VectorType::get(I64Ty, EltsOffset->getType()->getVectorNumElements()),
        "", LdI);
    Offset = BinaryOperator::CreateAdd(BaseOff, ZextOff, "", LdI);
  }
  Function *F = GenXIntrinsic::getGenXDeclaration(
      LdI->getModule(), IID,
      {OldValOfTheDataRead->getType(),
      Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType()});
  CallInst *Gather =
      m_useGlobalMem
          ? IntrinsicInst::Create(
                F, {Pred, logNumBlocks, Offset, OldValOfTheDataRead},
                LdI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, OldValOfTheDataRead},
                                  LdI->getName());
  Gather->insertAfter(LdI);
  m_gather.push_back(Gather);
  Instruction *ProperGather = RestoreVectorAfterNormalization(Gather, LdTy);

  if (!isa<VectorType>(LdI->getType()) &&
      isa<VectorType>(ProperGather->getType())) {
    Instruction *LdVal = CastInst::CreateBitOrPointerCast(ProperGather, LdI->getType());
    LdVal->insertAfter(ProperGather);
    ProperGather = LdVal;
  }

  LLVM_DEBUG(dbgs() << *Gather << "\n");
  LdI->replaceAllUsesWith(ProperGather);
  LdI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceStore(StoreInst *StI) {
  LLVM_DEBUG(dbgs() << "Replacing store " << *StI << " ===>\n");
  IRBuilder<> Builder(StI);
  Value *ValueOp = StI->getValueOperand();
  Type *ValueOpTy = ValueOp->getType();
  if (ValueOpTy->isIntOrPtrTy() || ValueOpTy->isFloatingPointTy()) {
    ValueOp = Builder.CreateVectorSplat(1, ValueOp);
    ValueOpTy = ValueOp->getType();
  }
  assert(ValueOp->getType()->isVectorTy());

  unsigned ValueEltSz = 0;
  std::tie(ValueOp, ValueEltSz) = NormalizeVector(ValueOp, ValueOpTy, StI);
  unsigned ValueNumElts = ValueOp->getType()->getVectorNumElements();

  Value *PointerOp = StI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);
  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Offset =
      ZExtOrTruncIfNeeded(Offset, m_useGlobalMem ? I64Ty : I32Ty, StI);

  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_scatter
                 : llvm::GenXIntrinsic::genx_scatter_scaled;

  Value *PredVal = ConstantInt::get(Type::getInt1Ty(*m_ctx), 1);
  Value *Pred = Builder.CreateVectorSplat(ValueNumElts, PredVal);
  Value *EltsOffset = FormEltsOffsetVector(ValueNumElts, ValueEltSz, StI);

  if (m_useGlobalMem && ValueNumElts > 1) {
    assert(Offset->getType()->getScalarType()->isIntegerTy(64));
    auto *BaseOff = FormEltsOffsetVectorForSVM(ValueNumElts, StI, Offset);
    auto *ZextOff = CastInst::CreateZExtOrBitCast(
        EltsOffset,
        VectorType::get(I64Ty, EltsOffset->getType()->getVectorNumElements()),
        "", StI);
    Offset = BinaryOperator::CreateAdd(BaseOff, ZextOff, "", StI);
  }

  Function *F = GenXIntrinsic::getGenXDeclaration(
      StI->getModule(), IID,
      {Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType(),
       ValueOp->getType()});
  Value *logNumBlocks = ConstantInt::get(I32Ty, m_useGlobalMem ? 0 : genx::log2(ValueEltSz));
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  auto *Scatter =
      m_useGlobalMem
          ? IntrinsicInst::Create(F, {Pred, logNumBlocks, Offset, ValueOp},
                                  StI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, ValueOp},
                                  StI->getName());
  Scatter->insertAfter(StI);
  StI->eraseFromParent();

  LLVM_DEBUG(dbgs() << *Scatter << "\n");
  m_scatter.push_back(Scatter);

  return true;
}

bool GenXThreadPrivateMemory::replacePTI(PtrToIntInst *PTI) {
  LLVM_DEBUG(dbgs() << "Replacing PTI " << *PTI << " ===> ");
  Value *PointerOp = PTI->getPointerOperand();
  Value *Offset = lookForPtrReplacement(PointerOp);

  if (isa<Argument>(Offset))
    return false;

  Offset = ZExtOrTruncIfNeeded(Offset, PTI->getDestTy(), PTI);
  LLVM_DEBUG(dbgs() << *Offset << "\n");
  PTI->replaceAllUsesWith(Offset);
  PTI->eraseFromParent();

  return true;
}

static Value *lookForTruncOffset(Value *V) {
  if (auto *I = dyn_cast<TruncInst>(V))
    return I->getOperand(0);
  else {
    // TODO: extend the list of supported instruction types
    if (auto *I = dyn_cast<BinaryOperator>(V)) {
      for (unsigned i = 0; i < I->getNumOperands(); ++i) {
        auto *Op = I->getOperand(i);
        if (auto *Off = lookForTruncOffset(Op)) {
          if (I->getType() != Off->getType()) {
            auto *OtherOp = I->getOperand((i + 1) % 2);
            OtherOp = ZExtOrTruncIfNeeded(OtherOp, Off->getType(), I);
            if (i == 0)
              I = BinaryOperator::Create(I->getOpcode(), Off, OtherOp,
                                         I->getName(), I);
            else
              I = BinaryOperator::Create(I->getOpcode(), OtherOp, Off,
                                         I->getName(), I);
          }
          return I;
        }
      }
    }
    return nullptr;
  }
}

bool GenXThreadPrivateMemory::replaceGatherPrivate(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "Replacing gather.priv " << *CI << " ===>\n");
  auto IID = m_useGlobalMem ? llvm::GenXIntrinsic::genx_svm_gather
                            : llvm::GenXIntrinsic::genx_gather_scaled;

  Type *OrigDstTy = CI->getType();
  assert(isa<VectorType>(OrigDstTy));
  Type *NewDstTy = OrigDstTy;
  Value *OldValue = CI->getArgOperand(3);
  unsigned ValueEltSz =
      m_DL->getTypeSizeInBits(NewDstTy->getScalarType()) / genx::ByteBits;

  // Check gather.private invariant.
  assert(NewDstTy == OldValue->getType());

  // Cast data type to legal.
  // Consider i64 legal for SVM cases
  if (!(m_useGlobalMem && CI->getType()->getScalarType()->isIntegerTy(64)))
    std::tie(OldValue, ValueEltSz) = NormalizeVector(OldValue, NewDstTy, CI);
  NewDstTy = OldValue->getType();
  unsigned ValueNumElts = NewDstTy->getVectorNumElements();

  Value *Pred = CI->getArgOperand(0);
  Value *EltsOffset = CI->getArgOperand(2);
  if (!m_useGlobalMem &&
      OrigDstTy->getVectorElementType()->getPrimitiveSizeInBits() ==
          genx::QWordBits) {
    assert(ValueNumElts == EltsOffset->getType()->getVectorNumElements() * 2);
    EltsOffset = DoubleVector(EltsOffset, ValueEltSz, CI);
    Pred = DoubleVector(Pred, 0, CI);
  }

  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Type *I64Ty = Type::getInt64Ty(*m_ctx);
  Value *PointerOp = CI->getOperand(1);
  Value *Offset = lookForPtrReplacement(PointerOp);
  Offset = ZExtOrTruncIfNeeded(Offset, I32Ty, CI);

  if (m_useGlobalMem) {
    if (!(Offset = lookForTruncOffset(EltsOffset)))
      Offset = CastInst::CreateZExtOrBitCast(
          EltsOffset,
          VectorType::get(I64Ty, EltsOffset->getType()->getVectorNumElements()),
          "", CI);
  }

  Function *F = GenXIntrinsic::getGenXDeclaration(
      CI->getModule(), IID,
      {NewDstTy, Pred->getType(),
       (m_useGlobalMem ? Offset : EltsOffset)->getType()});

  // 32u is max exec_size allowed (see GenXCisaBuilder.cpp:buildIntrinsic
  // GetExecSize lambda) For svm.gather/scatter:
  //    BlockSize is inferred from vec elem type
  //    BlockNum should be TotalMemSize / (ExecSize * BlockSize)
  //      where TotalMemSize is a total amount of mem read/written for
  //      gather/scatter
  // TODO: revise NumBlocks for non-svm case
  unsigned NumBlocks =
      (m_useGlobalMem)
          ? genx::log2(m_DL->getTypeSizeInBits(NewDstTy) /
                       (genx::ByteBits *
                        std::min(32u, NewDstTy->getVectorNumElements()) *
                        (m_DL->getTypeSizeInBits(NewDstTy->getScalarType()) /
                         genx::ByteBits)))
          : genx::log2(ValueEltSz);
  Value *logNumBlocks = ConstantInt::get(I32Ty, NumBlocks);
  Value *Scale = ConstantInt::get(Type::getInt16Ty(*m_ctx), 0);
  Value *Surface =
      ConstantInt::get(I32Ty, visa::getReservedSurfaceIndex(m_stack));

  CallInst *Gather =
      m_useGlobalMem
          ? IntrinsicInst::Create(F, {Pred, logNumBlocks, Offset, OldValue},
                                  CI->getName())
          : IntrinsicInst::Create(F,
                                  {Pred, logNumBlocks, Scale, Surface, Offset,
                                   EltsOffset, OldValue},
                                  CI->getName());
  Gather->insertAfter(CI);
  m_gather.push_back(Gather);
  LLVM_DEBUG(dbgs() << *Gather << "\n");

  Instruction *ProperGather =
      RestoreVectorAfterNormalization(Gather, OrigDstTy);
  CI->replaceAllUsesWith(ProperGather);
  CI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replaceScatterPrivate(CallInst *CI) {
  LLVM_DEBUG(dbgs() << "Replacing scatter.priv " << *CI << " ===>\n");
  auto IID = m_useGlobalMem
                 ? llvm::GenXIntrinsic::genx_svm_scatter
                 : llvm::GenXIntrinsic::genx_scatter_scaled;
  Value *ValueOp = CI->getArgOperand(3);
  Type *OrigValueTy = ValueOp->getType();
  assert(isa<VectorType>(OrigValueTy));
  unsigned EltSz = 0;
  std::tie(ValueOp, EltSz) = NormalizeVector(ValueOp, ValueOp->getType(), CI);

  Value *Pred = CI->getArgOperand(0);
  Value *EltsOffset = CI->getArgOperand(2);
  if (OrigValueTy->getVectorElementType()->getPrimitiveSizeInBits() ==
      genx::QWordBits) {
    EltsOffset = DoubleVector(EltsOffset, EltSz, CI);
    Pred = DoubleVector(Pred, 0, CI);
  }

  Value *ScatterPtr = CI->getArgOperand(1);
  Type *I32Ty = Type::getInt32Ty(*m_ctx);
  Value *Offset = lookForPtrReplacement(ScatterPtr);
  Offset = ZExtOrTruncIfNeeded(Offset, I32Ty, CI);

  Function *F = GenXIntrinsic::getGenXDeclaration(
      CI->getModule(), IID,
      {Pred->getType(), (m_useGlobalMem ? Offset : EltsOffset)->getType(),
       ValueOp->getType()});

  unsigned logNumBlocks = genx::log2(EltSz);
  unsigned Scale = 0; // scale is always 0
  Value *Surface = ConstantInt::get(I32Ty,
                                    visa::getReservedSurfaceIndex(m_stack));
  CallInst *ScatterStScaled =
      m_useGlobalMem
          ? IntrinsicInst::Create(
                F,
                {Pred, ConstantInt::get(I32Ty, logNumBlocks), Offset, ValueOp})
          : IntrinsicInst::Create(
                F, {Pred, ConstantInt::get(I32Ty, logNumBlocks),
                    ConstantInt::get(Type::getInt16Ty(*m_ctx), Scale), Surface,
                    Offset, EltsOffset, ValueOp});
  ScatterStScaled->insertAfter(CI);
  m_scatter.push_back(ScatterStScaled);
  LLVM_DEBUG(dbgs() << *ScatterStScaled << "\n");
  CI->replaceAllUsesWith(ScatterStScaled);
  CI->eraseFromParent();

  return true;
}

bool GenXThreadPrivateMemory::replacePhi(PHINode *Phi) {
  SmallVector<Value *, 8> PhiOps;
  for (auto &IncVal : Phi->incoming_values())
    PhiOps.push_back(lookForPtrReplacement(static_cast<Value *>(IncVal.get())));

  assert(!PhiOps.empty());

  // first we need to synchronize operands of types T and <1 x T> =>
  // make all of them scalar T
  auto NonVecOpIt = std::find_if(PhiOps.begin(), PhiOps.end(), [](Value *V) {
    return !V->getType()->isVectorTy();
  });
  if (NonVecOpIt != PhiOps.end()) {
    auto *NonVecTy = (*NonVecOpIt)->getType();

    auto TypeFixer = [NonVecTy, PhiOps](Value *&V) {
      if (V->getType() == NonVecTy)
        return;
      else if (V->getType()->getScalarType() == NonVecTy->getScalarType() &&
               V->getType()->isVectorTy() != NonVecTy->isVectorTy()) {
        if (V->getType()->isVectorTy()) {
          assert(V->getType()->getVectorNumElements() == 1);
          auto *VCast = CastInst::Create(CastInst::BitCast, V, NonVecTy->getScalarType());
          VCast->insertAfter(cast<Instruction>(V));
          V = VCast;
        }
      } else {
        assert(0 && "New phi types mismatch");
      }
    };
    std::for_each(PhiOps.begin(), PhiOps.end(), TypeFixer);
  }

  Type *OffsetTy = PhiOps[0]->getType();
  auto TypeChecker = [OffsetTy](Value *V) { return OffsetTy == V->getType(); };
  assert(std::all_of(PhiOps.begin(), PhiOps.end(), TypeChecker));

  PHINode *NewPhi = PHINode::Create(OffsetTy, PhiOps.size());
  for (unsigned i = 0; i < PhiOps.size(); ++i)
    NewPhi->addIncoming(PhiOps[i], Phi->getIncomingBlock(i));

  NewPhi->insertAfter(Phi);

  // Create temporary cast instruction to satisfy old phi users. Types must be
  // different due to replacement pointer by integer offset.
  assert(NewPhi->getType() != Phi->getType());
  CastInst *TempCast = CastInst::CreateBitOrPointerCast(NewPhi, Phi->getType());
  TempCast->insertAfter(NewPhi->getParent()->getFirstNonPHI());

  Phi->replaceAllUsesWith(TempCast);
  Phi->eraseFromParent();

  return true;
}

// |--%1 = PHI(%2, ...)
// |         ^
// |         |
// |         |
// |  %2 = PHI(%1, ...)
// |---------^
//
// In this situation, it's difficult to find the origin of the pointer. PtrToInt
// and IntToPtr break the process of searching (see lookForPtrReplacement) and
// it helps to 'emulate' phi in TPM
bool GenXThreadPrivateMemory::preparePhiForReplacement(PHINode *Phi) {
  if (!isa<PointerType>(Phi->getType()))
    return false;

  Type *I64Ty = Type::getInt64Ty(Phi->getContext());
  StringRef Name = Phi->getName();
  Instruction *TempPtrToInt = CastInst::Create(
      Instruction::PtrToInt, Phi, I64Ty, Name + ".tpm.temp.pti",
      Phi->getParent()->getFirstNonPHI());
  Instruction *TempIntToPtr =
      CastInst::Create(Instruction::IntToPtr, TempPtrToInt, Phi->getType(),
                       Name + ".tpm.temp.itp");
  TempIntToPtr->insertAfter(TempPtrToInt);
  Phi->replaceAllUsesWith(TempIntToPtr);

  // Replacement here was incorrect
  TempPtrToInt->replaceUsesOfWith(TempIntToPtr, Phi);

  return true;
}

bool GenXThreadPrivateMemory::replaceSelect(SelectInst *Sel) {
  Value *Cond = Sel->getCondition();
  Value *TrueValue = lookForPtrReplacement(Sel->getTrueValue());
  Value *FalseValue = lookForPtrReplacement(Sel->getFalseValue());

  SelectInst *NewSel = SelectInst::Create(Cond, TrueValue, FalseValue);
  NewSel->insertAfter(Sel);
  NewSel->setDebugLoc(Sel->getDebugLoc());

  CastInst *TempCast = CastInst::CreateBitOrPointerCast(NewSel, Sel->getType());
  TempCast->insertAfter(NewSel);
  TempCast->setDebugLoc(Sel->getDebugLoc());

  Sel->replaceAllUsesWith(TempCast);
  Sel->eraseFromParent();

  return true;
}

static Value *GetUndefVec(Type *Ty, unsigned NumElts) {
  return UndefValue::get(VectorType::get(Ty, NumElts));
}

static std::pair<Value *, Value *> GetUndefPair(Type *Ty, unsigned NumElts) {
  return std::make_pair(GetUndefVec(Ty, NumElts), GetUndefVec(Ty, NumElts));
}

static Value *FillVecWithSeqVals(Value *Vec, unsigned Start,
                                 Instruction *InsertBefore) {
  IRBuilder<> Builder(InsertBefore);
  Builder.SetInsertPoint(InsertBefore);

  Type *I32Ty = Type::getInt32Ty(InsertBefore->getContext());
  unsigned NumElts = Vec->getType()->getVectorNumElements();
  for (unsigned i = 0; i < NumElts; ++i) {
    Value *Idx = ConstantInt::get(I32Ty, i);
    Value *Val = ConstantInt::get(I32Ty, i + Start);
    Vec = Builder.CreateInsertElement(Vec, Val, Idx);
  }
  return Vec;
}

static std::pair<Value *, Value *>
SplitVec(Value *Vec, unsigned NumElts, Instruction *InsertBefore,
         std::pair<Value *, Value *> Splitters) {
  IRBuilder<> Builder(InsertBefore);
  Builder.SetInsertPoint(InsertBefore);

  Type *EltTy = Vec->getType()->getVectorElementType();
  Value *First = Builder.CreateShuffleVector(Vec, GetUndefVec(EltTy, NumElts),
                                             Splitters.first);
  Value *Second = Builder.CreateShuffleVector(Vec, GetUndefVec(EltTy, NumElts),
                                              Splitters.second);
  return std::make_pair(First, Second);
}

void SplitScatter(CallInst *CI) {
  assert(GenXIntrinsic::getAnyIntrinsicID(CI) ==
         llvm::GenXIntrinsic::genx_scatter_scaled);
  Type *DataTy = CI->getArgOperand(5)->getType();
  unsigned NumElts = DataTy->getVectorNumElements();
  assert(NumElts % 2 == 0);

  Type *I32Ty = Type::getInt32Ty(CI->getContext());
  std::pair<Value *, Value *> Splitters = GetUndefPair(I32Ty, NumElts / 2);
  Splitters.first = FillVecWithSeqVals(Splitters.first, 0, CI);
  Splitters.second = FillVecWithSeqVals(Splitters.second, NumElts / 2, CI);

  Value *Pred = CI->getArgOperand(0);
  std::pair<Value *, Value *> NewPreds = SplitVec(Pred, NumElts, CI, Splitters);

  Value *EltOffsets = CI->getArgOperand(5);
  std::pair<Value *, Value *> NewEltOffsets =
      SplitVec(EltOffsets, NumElts, CI, Splitters);

  Value *OldVal = CI->getArgOperand(6);
  std::pair<Value *, Value *> OldVals =
      SplitVec(OldVal, NumElts, CI, Splitters);

  auto IID = llvm::GenXIntrinsic::genx_scatter_scaled;
  Function *F = GenXIntrinsic::getGenXDeclaration(CI->getModule(), IID,
                                          {NewPreds.first->getType(),
                                           NewEltOffsets.first->getType(),
                                           OldVals.first->getType()});

  Value *LogNumBlock = CI->getArgOperand(1);
  Value *Scale = CI->getArgOperand(2);
  Value *Surface = CI->getArgOperand(3);
  Value *Offset = CI->getArgOperand(4);

  CallInst *FirstScatter =
      IntrinsicInst::Create(F, {NewPreds.first, LogNumBlock, Scale, Surface,
                                Offset, NewEltOffsets.first, OldVals.first});
  CallInst *SecondScatter =
      IntrinsicInst::Create(F, {NewPreds.second, LogNumBlock, Scale, Surface,
                                Offset, NewEltOffsets.second, OldVals.second});

  FirstScatter->insertAfter(CI);
  SecondScatter->insertAfter(FirstScatter);

  CI->eraseFromParent();
}

void SplitGather(CallInst *CI) {
  assert(GenXIntrinsic::getAnyIntrinsicID(CI) ==
         llvm::GenXIntrinsic::genx_gather_scaled);
  Type *DstTy = CI->getType();
  unsigned NumElts = DstTy->getVectorNumElements();
  assert(NumElts % 2 == 0);

  Type *I32Ty = Type::getInt32Ty(CI->getContext());
  std::pair<Value *, Value *> Splitters = GetUndefPair(I32Ty, NumElts / 2);
  Splitters.first = FillVecWithSeqVals(Splitters.first, 0, CI);
  Splitters.second = FillVecWithSeqVals(Splitters.second, NumElts / 2, CI);

  Value *Pred = CI->getArgOperand(0);
  std::pair<Value *, Value *> NewPreds = SplitVec(Pred, NumElts, CI, Splitters);

  Value *EltOffsets = CI->getArgOperand(5);
  std::pair<Value *, Value *> NewEltOffsets =
      SplitVec(EltOffsets, NumElts, CI, Splitters);

  Value *OldVal = CI->getArgOperand(6);
  std::pair<Value *, Value *> OldVals =
      SplitVec(OldVal, NumElts, CI, Splitters);
  auto IID = llvm::GenXIntrinsic::genx_gather_scaled;
  Function *F = GenXIntrinsic::getGenXDeclaration(CI->getModule(), IID,
                                          {OldVals.first->getType(),
                                           NewPreds.first->getType(),
                                           NewEltOffsets.first->getType()});

  Value *LogNumBlock = CI->getArgOperand(1);
  Value *Scale = CI->getArgOperand(2);
  Value *Surface = CI->getArgOperand(3);
  Value *Offset = CI->getArgOperand(4);

  CallInst *FirstGather =
      IntrinsicInst::Create(F, {NewPreds.first, LogNumBlock, Scale, Surface,
                                Offset, NewEltOffsets.first, OldVals.first});
  CallInst *SecondGather =
      IntrinsicInst::Create(F, {NewPreds.second, LogNumBlock, Scale, Surface,
                                Offset, NewEltOffsets.second, OldVals.second});

  FirstGather->insertAfter(CI);
  SecondGather->insertAfter(FirstGather);

  Value *Joiner = FillVecWithSeqVals(GetUndefVec(I32Ty, NumElts), 0, CI);
  IRBuilder<> Builder(CI);
  Builder.SetInsertPoint(SecondGather->getNextNode());
  Value *JointGather =
      Builder.CreateShuffleVector(FirstGather, SecondGather, Joiner);

  CI->replaceAllUsesWith(JointGather);
  CI->eraseFromParent();
}

class SVMChecker {
  std::map<Value *, int> Visited;

public:
  // pre-transformation analysis to determine
  // which kind of mem should we place TPM at
  int checkSVMNecessary(Value *V) {
    if (Visited.count(V) > 0)
      return Visited.at(V);
    // do not handle ConstExprs for now
    if (!isa<Instruction>(V) && !isa<Argument>(V))
      return 0;
    int LoadsMet = 0;
    if (isa<LoadInst>(V)) {
      ++LoadsMet;
    } else if (auto *CI = dyn_cast<CallInst>(V)) {
      auto IID = GenXIntrinsic::getAnyIntrinsicID(CI);
      if (IID == GenXIntrinsic::genx_gather_private ||
          IID == GenXIntrinsic::genx_scatter_private ||
          IID == GenXIntrinsic::not_any_intrinsic) {
        // do not process users of priv mem intrinsics
        // or calls to other functions
        return 0;
      }
    } else if (isa<PHINode>(V) || isa<ICmpInst>(V)) {
      // do not go thru phi as loops may appear and
      // it doesn't seem necessary for the analysis now
      return 0;
    }
    int Result = 0;
    for (auto *U : V->users()) {
      Result = std::max(Result, checkSVMNecessary(U));
    }
    Visited.insert(std::make_pair(V, Result + LoadsMet));
    return Result + LoadsMet;
  }

  bool operator()(Value *V) { return checkSVMNecessary(V) > 1; }
};

void GenXThreadPrivateMemory::addUsers(Value *V) {
  assert(isa<Instruction>(V) || isa<Argument>(V));
  for (const auto &Usr : V->users()) {
    Instruction *ToAdd = cast<Instruction>(Usr);
    auto Found = m_AlreadyAdded.find(ToAdd);
    if (Found == m_AlreadyAdded.end()) {
      m_AlreadyAdded.insert(ToAdd);
      m_AIUsers.push(ToAdd);
    }
  }
}

void GenXThreadPrivateMemory::collectEachPossibleTPMUsers() {
  assert(m_AIUsers.empty());
  // At first collect every alloca user
  for (auto B = m_allocaToIntrinsic.begin(), E = m_allocaToIntrinsic.end();
       B != E; ++B) {
    Instruction *I = dyn_cast<Instruction>(B->first);
    assert(I);
    addUsers(I);
  }
  // Then collect all pointer args - they may be used
  // in loads/stores we need to lower to svm intrinsics
  // m_args already contatins only args that require processing
  for (auto &Arg : m_args)
    addUsers(Arg);
}

void GenXThreadPrivateMemory::addUsersIfNeeded(Value *V) {
  bool isGatherScatterPrivate = false;
  if (IntrinsicInst *CI = dyn_cast<IntrinsicInst>(V)) {
    unsigned ID = GenXIntrinsic::getAnyIntrinsicID(CI);
    switch (ID) {
    case GenXIntrinsic::genx_gather_private:
    case GenXIntrinsic::genx_scatter_private:
    case Intrinsic::lifetime_start:
    case Intrinsic::lifetime_end:
      isGatherScatterPrivate = true;
      break;
    default:
      break;
    }
  }
  if (!isa<LoadInst>(V) && !isa<StoreInst>(V) &&
      V->getType()->getScalarType()->isIntegerTy(1))
    return;
  if (m_useGlobalMem ||
      (!isa<LoadInst>(V) && !isa<StoreInst>(V) && !isGatherScatterPrivate))
    addUsers(V);
}

bool GenXThreadPrivateMemory::runOnModule(Module &M) {
  auto STP = getAnalysisIfAvailable<GenXSubtargetPass>();
  assert(STP);
  m_ST = STP->getSubtarget();
  for (auto &F : M)
    visit(F);
  if (!m_useGlobalMem &&
      std::find_if(m_alloca.begin(), m_alloca.end(), SVMChecker()) !=
          m_alloca.end()) {
    LLVM_DEBUG(dbgs() << "Switching TPM to SVM\n");
    // TODO: move the name string to vc-intrinsics *MD::useGlobalMem
    M.addModuleFlag(Module::ModFlagBehavior::Error, "genx.useGlobalMem", 1);
    m_useGlobalMem = true;
  }
  bool Result = false;
  for (auto &F : M)
    Result |= runOnFunction(F);
  return Result;
}

bool GenXThreadPrivateMemory::runOnFunction(Function &F) {
  // skip function which is not a kernel or stackfunc
  // typically it's an emulation-related func (__cm_intrinsic_impl_*)
  if (GenXIntrinsic::getAnyIntrinsicID(&F) !=
          GenXIntrinsic::not_any_intrinsic ||
      !(F.hasFnAttribute(genx::FunctionMD::CMStackCall) ||
        F.hasFnAttribute(genx::FunctionMD::CMGenXMain)))
    return false;
  LLVM_DEBUG(dbgs() << "Running TPM on " << F.getName() << "\n");
  m_DL = &F.getParent()->getDataLayout();
  m_stack = m_ST->stackSurface();

  m_ctx = &F.getContext();
  m_DL = &F.getParent()->getDataLayout();
  m_alloca.clear();
  m_args.clear();
  m_gather.clear();
  m_scatter.clear();
  m_allocaToIntrinsic.clear();
  m_AIUsers = {};
  m_AlreadyAdded.clear();

  visit(F);

  for (auto Alloca : m_alloca) {
    Type *AllocaTy = Alloca->getAllocatedType();

    auto IID = llvm::GenXIntrinsic::genx_alloca;
    Function *IntrDecl = GenXIntrinsic::getGenXDeclaration(Alloca->getModule(), IID, AllocaTy);
    CallInst *AllocaIntr =
        IntrinsicInst::Create(IntrDecl, {Constant::getNullValue(AllocaTy)});
    AllocaIntr->insertAfter(Alloca);
    m_allocaToIntrinsic[Alloca] = AllocaIntr;
  }

  // Firstly, we resolve dependencies in PHI nodes (see comments in
  // preparePhiForReplacement).
  collectEachPossibleTPMUsers();
  bool Changed = false;
  while (!m_AIUsers.empty()) {
    Instruction *I = m_AIUsers.front();
    m_AIUsers.pop();

    addUsersIfNeeded(I);

    if (PHINode *Phi = dyn_cast<PHINode>(I))
      Changed |= preparePhiForReplacement(Phi);
  }

  // Main loop where instructions are replaced one by one.
  m_AlreadyAdded.clear();
  collectEachPossibleTPMUsers();
  while (!m_AIUsers.empty()) {
    Instruction *I = m_AIUsers.front();
    LLVM_DEBUG(dbgs() << "Processing inst: " << *I << "\n");
    m_AIUsers.pop();

    addUsersIfNeeded(I);

    if (auto *LdI = dyn_cast<LoadInst>(I))
      Changed |= replaceLoad(LdI);
    else if (auto *StI = dyn_cast<StoreInst>(I))
      Changed |= replaceStore(StI);
    else if (auto *PTI = dyn_cast<PtrToIntInst>(I))
      Changed |= replacePTI(PTI);
    else if (auto* AddrCast = dyn_cast<AddrSpaceCastInst>(I))
      Changed |= replaceAddrSpaceCast(AddrCast);
    else if (isa<IntToPtrInst>(I) || isa<BitCastInst>(I)) {
      // resolve all IntToPtr users and remove it.
      if (I->use_empty()) {
        I->eraseFromParent();
        Changed = true;
      }
    } else if (IntrinsicInst *CI = dyn_cast<IntrinsicInst>(I)) {
      unsigned ID = GenXIntrinsic::getAnyIntrinsicID(CI);
      if (ID == GenXIntrinsic::genx_gather_private)
        Changed |= replaceGatherPrivate(CI);
      else if (ID == GenXIntrinsic::genx_scatter_private)
        Changed |= replaceScatterPrivate(CI);
      else if (ID == Intrinsic::lifetime_start ||
               ID == Intrinsic::lifetime_end) {
        CI->eraseFromParent();
        Changed = true;
      }
    } else if (PHINode *Phi = dyn_cast<PHINode>(I)) {
      if (isa<PointerType>(Phi->getType()))
        Changed |= replacePhi(Phi);
    } else if (SelectInst *Sel = dyn_cast<SelectInst>(I)) {
      if (isa<PointerType>(Sel->getType()))
        Changed |= replaceSelect(Sel);
    }

    if (m_AIUsers.empty()) {
      if (!Changed)
        report_fatal_error("Thread private memory: cannot resolve all alloca uses");
      Changed = false;
      collectEachPossibleTPMUsers();
    }
  }

  for (auto AllocaPair : m_allocaToIntrinsic) {
    while (!AllocaPair.first->user_empty()) {
      const auto &U = AllocaPair.first->user_back();
      assert(U->getNumUses() == 0);
      cast<Instruction>(U)->eraseFromParent();
    }
    assert(AllocaPair.first->use_empty() &&
           "uses of replaced alloca aren't empty");
    AllocaPair.first->eraseFromParent();
  }

  // TODO: Rewrite split conditions due to possible exec sizes are 1, 2, 4, 8,
  // 16 and 32.
  for (auto CI : m_gather) {
    Type *DstTy = CI->getType();
    unsigned NumElts = DstTy->getVectorNumElements();
    unsigned EltSz = DstTy->getVectorElementType()->getPrimitiveSizeInBits();
    unsigned ExecSz = NumElts * EltSz;

    if (ExecSz > 2 * genx::GRFBits || NumElts > 32)
      SplitGather(CI);
  }

  for (auto CI : m_scatter) {
    Type *DataTy =
        CI->getArgOperand(m_useGlobalMem ? 3 : 5)->getType();
    unsigned NumElts = DataTy->getVectorNumElements();
    unsigned EltSz = DataTy->getVectorElementType()->getPrimitiveSizeInBits();
    unsigned ExecSz = NumElts * EltSz;

    if (ExecSz > 2 * genx::GRFBits || NumElts > 32)
      SplitScatter(CI);
  }

  return !m_allocaToIntrinsic.empty();
}

void GenXThreadPrivateMemory::visitAllocaInst(AllocaInst &I) {
  m_alloca.push_back(&I);
}

void GenXThreadPrivateMemory::visitFunction(Function &F) {
  for (auto &Arg : F.args())
    if (Arg.getType()->isPointerTy() && SVMChecker()(&Arg)) {
      LLVM_DEBUG(dbgs() << "Switching TPM to SVM: svm arg\n");
      // TODO: move the name string to vc-intrinsics *MD::useGlobalMem
      if (!m_useGlobalMem)
        F.getParent()->addModuleFlag(Module::ModFlagBehavior::Error,
                                     "genx.useGlobalMem", 1);
      m_useGlobalMem = true;
      m_args.push_back(&Arg);
    }
}
