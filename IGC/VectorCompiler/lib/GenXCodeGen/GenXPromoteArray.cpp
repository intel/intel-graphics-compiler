/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

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

#include "llvm/ADT/SmallVector.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/Support/Alignment.h"
#include "llvmWrapper/Support/TypeSize.h"

#include <algorithm>
#include <queue>
#include <vector>

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

// The class preserves index into a vector and the size of an element
// of this vector.
// The idea is that vector can change throughout bitcasts and its index
// and element size should change correspondingly.
// A product of Index and ElementSizeInBits gives an offset in bits of
// a considered element in a considered vector.
struct GenericVectorIndex {
  Value *Index;
  int ElementSizeInBits;

  int getElementSizeInBytes() const {
    return ElementSizeInBits / genx::ByteBits;
  }
};

// Diagnostic information for error/warning relating array promotion.
class DiagnosticInfoPromoteArray : public DiagnosticInfo {
private:
  std::string Description;

  static const int KindID;

  static int getKindID() { return KindID; }

public:
  // Initialize from description
  DiagnosticInfoPromoteArray(const Twine &Desc,
                             DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(getKindID(), Severity), Description(Desc.str()) {}

  void print(DiagnosticPrinter &DP) const override {
    DP << "GenXPromoteArray: " << Description;
  }
};

const int DiagnosticInfoPromoteArray::KindID =
    llvm::getNextAvailablePluginDiagnosticKind();

class TransposeHelper {
public:
  TransposeHelper(bool vectorIndex, const llvm::DataLayout *DL)
      : m_vectorIndex(vectorIndex), m_pDL(DL) {}
  void handleAllocaSources(Instruction &Inst, GenericVectorIndex Idx);
  void handleGEPInst(GetElementPtrInst *pGEP, GenericVectorIndex Idx);
  void handleBCInst(BitCastInst &BC, GenericVectorIndex Idx);
  void handlePTIInst(PtrToIntInst &BC, GenericVectorIndex Idx);
  void handlePHINode(PHINode *pPhi, GenericVectorIndex pScalarizedIdx,
                     BasicBlock *pIncomingBB);
  virtual void handleLoadInst(llvm::LoadInst *pLoad,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleStoreInst(llvm::StoreInst *pStore,
                               GenericVectorIndex pScalarizedIdx) = 0;
  virtual void handlePrivateGather(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handlePrivateScatter(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleSVMGather(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleSVMScatter(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleLLVMGather(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleLLVMScatter(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  void EraseDeadCode();

private:
  bool m_vectorIndex = false;
  std::vector<llvm::Instruction *> m_toBeRemoved;
  ValueMap<llvm::PHINode*, llvm::PHINode*> m_phiReplacement;

protected:
  const llvm::DataLayout *m_pDL = nullptr;
};

/// @brief  TransformPrivMem pass is used for lowering the allocas identified
/// while visiting the alloca instructions
///         and then inserting insert/extract elements instead of load stores.
///         This allows us to store the data in registers instead of propagating
///         it to scratch space.
class TransformPrivMem : public llvm::FunctionPass,
                         public llvm::InstVisitor<TransformPrivMem> {
public:
  TransformPrivMem();

  ~TransformPrivMem() {}

  llvm::StringRef getPassName() const override { return "TransformPrivMem"; }

  void getAnalysisUsage(AnalysisUsage &AU) const override {
    AU.addRequired<GenXBackendConfig>();
    AU.setPreservesCFG();
  }

  bool runOnFunction(llvm::Function &F) override;

  void visitAllocaInst(llvm::AllocaInst &I);

  void visitStore(llvm::StoreInst &St);

  unsigned int extractAllocaSize(llvm::AllocaInst *pAlloca);

private:
  llvm::AllocaInst *createVectorForAlloca(llvm::AllocaInst *pAlloca,
                                          llvm::Type *pBaseType);
  void handleAllocaInst(llvm::AllocaInst *pAlloca);

  void selectAllocasToHandle();
  bool CheckIfAllocaPromotable(AllocaInst &pAlloca);

  bool replaceSingleAggrStore(llvm::StoreInst *StI);

  bool replaceAggregatedStore(llvm::StoreInst *StI);

public:
  static char ID;

private:
  std::queue<StoreInst *> m_StoresToHandle;
  const llvm::DataLayout *m_pDL = nullptr;
  LLVMContext *m_ctx = nullptr;
  std::vector<llvm::AllocaInst *> m_allocasToPrivMem;
  llvm::Function *m_pFunc = nullptr;
  bool ForcePromotion = false;
  bool LargeAllocasWereLeft = false;
};
} // namespace

// Register pass to igc-opt
namespace llvm {
void initializeTransformPrivMemPass(PassRegistry &);
}
#define PASS_FLAG "transform-priv-mem"
#define PASS_DESCRIPTION                                                       \
  "transform private arrays for promoting them to registers"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
INITIALIZE_PASS_BEGIN(TransformPrivMem, PASS_FLAG, PASS_DESCRIPTION,
                      PASS_CFG_ONLY, PASS_ANALYSIS)
INITIALIZE_PASS_DEPENDENCY(GenXBackendConfig)
INITIALIZE_PASS_END(TransformPrivMem, PASS_FLAG, PASS_DESCRIPTION,
                    PASS_CFG_ONLY, PASS_ANALYSIS)

char TransformPrivMem::ID = 0;

FunctionPass *llvm::createTransformPrivMemPass() {
  return new TransformPrivMem();
}

namespace {

class TransposeHelperPromote : public TransposeHelper {
public:
  void handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx) override;
  void handleStoreInst(StoreInst *pStore, GenericVectorIndex pScalarizedIdx) override;
  void handlePrivateGather(IntrinsicInst *pInst, Value *pScalarizedIdx) override;
  void handlePrivateScatter(IntrinsicInst *pInst, Value *pScalarizedIdx) override;
  void handleSVMGather(IntrinsicInst *pInst, Value *pScalarizedIdx) override;
  void handleSVMScatter(IntrinsicInst *pInst, Value *pScalarizedIdx) override;
  void handleLLVMGather(IntrinsicInst *pInst, Value *pScalarizedIdx) override;
  void handleLLVMScatter(IntrinsicInst *pInst, Value *pScalarizedIdx) override;

  AllocaInst *pVecAlloca;

  TransposeHelperPromote(AllocaInst *pAI, const llvm::DataLayout *DL)
      : TransposeHelper(false, DL) {
    pVecAlloca = pAI;
  }
};

TransformPrivMem::TransformPrivMem() : FunctionPass(ID), m_pFunc(nullptr) {
  initializeTransformPrivMemPass(*PassRegistry::getPassRegistry());
}

static IGCLLVM::FixedVectorType &
getVectorTypeForAlloca(AllocaInst &Alloca, Type &ElemTy, const DataLayout &DL) {
  auto AllocaSize = Alloca.getAllocationSizeInBits(DL);
  IGC_ASSERT_MESSAGE(AllocaSize.hasValue(), "VLA is not expected");
  auto NumElem = AllocaSize.getValue() / DL.getTypeAllocSizeInBits(&ElemTy);
  return *IGCLLVM::FixedVectorType::get(&ElemTy, NumElem);
}

llvm::AllocaInst *
TransformPrivMem::createVectorForAlloca(llvm::AllocaInst *pAlloca,
                                        llvm::Type *pBaseType) {
  IRBuilder<> IRB(pAlloca);
  auto &VecType = getVectorTypeForAlloca(*pAlloca, *pBaseType, *m_pDL);
  AllocaInst *pAllocaValue = IRB.CreateAlloca(&VecType);
  return pAllocaValue;
}

bool TransformPrivMem::replaceSingleAggrStore(StoreInst *StI) {
  IRBuilder<> Builder(StI);

  Value *ValueOp = StI->getValueOperand();
  Value *Ptr = StI->getPointerOperand();
  unsigned AS = StI->getPointerAddressSpace();
  Value *ValToStore = Builder.CreateExtractValue(ValueOp, 0);
  ValToStore->setName(ValueOp->getName() + ".noAggr");

  StoreInst *NewStI = Builder.CreateAlignedStore(ValToStore,
    Builder.CreateBitCast(Ptr, ValToStore->getType()->getPointerTo(AS)),
    IGCLLVM::getAlign(StI->getAlignment()), StI->isVolatile());
  m_StoresToHandle.push(NewStI);
  StI->eraseFromParent();

  return true;
}

bool TransformPrivMem::replaceAggregatedStore(StoreInst *StI) {
  IRBuilder<> Builder(StI);
  Value *ValueOp = StI->getValueOperand();
  Type *ValueOpTy = ValueOp->getType();
  auto *ST = dyn_cast<StructType>(ValueOpTy);
  auto *AT = dyn_cast<ArrayType>(ValueOpTy);

  IGC_ASSERT(StI->isSimple());
  IGC_ASSERT(AT || ST);

  uint64_t Count = ST ? ST->getNumElements() : AT->getNumElements();
  if (Count == 1) {
    return replaceSingleAggrStore(StI);
  }

  auto *IdxType = Type::getInt32Ty(*m_ctx);
  auto *Zero = ConstantInt::get(IdxType, 0);
  for (uint64_t i = 0; i < Count; ++i) {
    Value *Indices[2] = {
      Zero,
      ConstantInt::get(IdxType, i)
    };

    Value *Ptr = nullptr;
    auto *PtrOp = StI->getPointerOperand();
    if (ST) {
      Ptr = Builder.CreateInBoundsGEP(ST,
        PtrOp, makeArrayRef(Indices));
    } else {
      Ptr = Builder.CreateInBoundsGEP(AT,
        PtrOp, makeArrayRef(Indices));
    }
    Ptr->setName(PtrOp->getName() + ".noAggrGEP");
    auto *Val = Builder.CreateExtractValue(ValueOp, i);
    Val->setName(ValueOp->getName() + ".noAggr");
    StoreInst *NewStI = Builder.CreateStore(Val, Ptr, StI->isVolatile());

    m_StoresToHandle.push(NewStI);
  }

  StI->eraseFromParent();

  return true;
}

bool TransformPrivMem::runOnFunction(llvm::Function &F) {
  m_pFunc = &F;
  m_ctx = &(m_pFunc->getContext());

  m_pDL = &F.getParent()->getDataLayout();
  ForcePromotion = getAnalysis<GenXBackendConfig>().isArrayPromotionForced() &&
                   TotalAllocaLimitOpt.getNumOccurrences() == 0 &&
                   SingleAllocaLimitOpt.getNumOccurrences() == 0;
  LargeAllocasWereLeft = false;
  m_allocasToPrivMem.clear();

  visit(F);

  bool AggrRemoved = false;
  while (!m_StoresToHandle.empty()) {
    StoreInst *StI = m_StoresToHandle.front();
    m_StoresToHandle.pop();
    if (StI->getValueOperand()->getType()->isAggregateType())
      AggrRemoved |= replaceAggregatedStore(StI);
  }

  selectAllocasToHandle();

  if (LargeAllocasWereLeft)
    vc::warn(vc::WarningName::Generic, F.getContext(), *this,
             F.getName() + " allocation size is too big: using TPM");

  for (auto *Alloca : m_allocasToPrivMem) {
    handleAllocaInst(Alloca);
  }

  // Last remove alloca instructions
  for (auto *pInst : m_allocasToPrivMem) {
    if (pInst->use_empty()) {
      pInst->eraseFromParent();
    }
  }
  // IR changed only if we had alloca instruction to optimize or
  // if aggregated stores were replaced
  return !m_allocasToPrivMem.empty() || AggrRemoved;
}

unsigned int TransformPrivMem::extractAllocaSize(llvm::AllocaInst *pAlloca) {
  unsigned int arraySize =
      (unsigned int)(cast<ConstantInt>(pAlloca->getArraySize())
                         ->getZExtValue());
  unsigned int totalArrayStructureSize =
      (unsigned int)(m_pDL->getTypeAllocSize(pAlloca->getAllocatedType()) *
                     arraySize);

  return totalArrayStructureSize;
}

static Type *GetBaseType(Type *pType, Type *pBaseType) {
  while (pType->isStructTy() || pType->isArrayTy() || pType->isVectorTy()) {
    if (pType->isStructTy()) {
      int num_elements = pType->getStructNumElements();
      for (int i = 0; i < num_elements; ++i) {
        Type *structElemBaseType =
            GetBaseType(pType->getStructElementType(i), pBaseType);
        // can support only homogeneous structures
        if (pBaseType != nullptr &&
            (structElemBaseType == nullptr ||
             structElemBaseType != pBaseType))
          return nullptr;
        pBaseType = structElemBaseType;
      }
      return pBaseType;
    } else if (pType->isArrayTy()) {
      pType = pType->getArrayElementType();
    } else if (pType->isVectorTy()) {
      pType = cast<VectorType>(pType)->getElementType();
    } else {
      IGC_ASSERT(0);
    }
  }
  if (pType->isPointerTy() && pType->getPointerElementType()->isFunctionTy())
    pType = IntegerType::getInt64Ty(pType->getContext());
  return pType;
}

static bool CheckPtrToIntCandidate(PtrToIntInst *PTI) {
  // here we handle only the most common pattern for SVM instructions
  // ptrtoint->insertelem->shuffle->arith_op->svm_gather/scatter
  // others are possible, but not handled yet
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
  for (auto *MemOp : BinOp->users()) {
    if (!isa<CallInst>(MemOp))
      return false;
    auto IID = vc::getAnyIntrinsicID(MemOp);
    if (IID != GenXIntrinsic::genx_svm_gather &&
        IID != GenXIntrinsic::genx_svm_scatter)
      return false;
    // for now skip insts w/ blockSize > 1
    // or weird things like <16 x i32> %res = svm.gather(<8 x i64> offsets, ...)
    auto *Pred = MemOp->getOperand(0);
    auto *NumBlocks = MemOp->getOperand(1);
    auto *Input = MemOp->getOperand(3);
    // ignore reads of types different from alloca types, e.g.
    // %v0 = alloca [16 x i8]
    // .. store of global to %v0
    // %offsets = %v0 + <0, 4, 8, 12, 0, 4, 8, 12>
    // ....
    // %v1 = <8 x float> svm_gather %v0, %offsets, <8 x float> undef
    // OR
    // svm_scatter %v0, %offset, <8 x float> %value
    if (Input->getType()->getScalarType() !=
        GetBaseType(PTI->getOperand(0)->getType()->getPointerElementType(),
                    nullptr))
      return false;

    IGC_ASSERT(isa<ConstantInt>(NumBlocks));
    if (cast<ConstantInt>(NumBlocks)->getZExtValue() ||
        cast<IGCLLVM::FixedVectorType>(Input->getType())->getNumElements() >
            cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements() ||
        (isa<VectorType>(MemOp->getType()) &&
         cast<IGCLLVM::FixedVectorType>(Pred->getType())->getNumElements() <
             cast<IGCLLVM::FixedVectorType>(MemOp->getType())
                 ->getNumElements()))
      return false;
  }
  return true;
}

static bool CheckAllocaUsesInternal(Instruction *I) {
  for (Value::user_iterator use_it = I->user_begin(), use_e = I->user_end();
       use_it != use_e; ++use_it) {
    if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*use_it)) {
      auto PtrV = gep->getPointerOperand();
      // we cannot support a vector of pointers as the base of the GEP
      if (PtrV->getType()->isPointerTy()) {
        if (CheckAllocaUsesInternal(gep))
          continue;
      }
      return false;
    }
    if (llvm::LoadInst *pLoad = llvm::dyn_cast<llvm::LoadInst>(*use_it)) {
      if (!pLoad->isSimple())
        return false;
    } else if (llvm::StoreInst *pStore =
                   llvm::dyn_cast<llvm::StoreInst>(*use_it)) {
      if (!pStore->isSimple())
        return false;
      llvm::Value *pValueOp = pStore->getValueOperand();
      if (pValueOp == I) {
        // GEP instruction is the stored value of the StoreInst (not supported
        // case)
        return false;
      }
    } else if (llvm::BitCastInst *pBitCast =
                   llvm::dyn_cast<llvm::BitCastInst>(*use_it)) {
      if (pBitCast->use_empty())
        continue;
      Type *baseT = GetBaseType(
          pBitCast->getType()->getScalarType()->getPointerElementType(),
          nullptr);
      Type *sourceType = GetBaseType(pBitCast->getOperand(0)
                                         ->getType()
                                         ->getScalarType()
                                         ->getPointerElementType(),
                                     nullptr);
      IGC_ASSERT(sourceType);
      // either the point-to-element-type is the same or
      // the point-to-element-type is the byte or a function pointer
      if (baseT != nullptr &&
          (baseT->getScalarSizeInBits() == 8 ||
           baseT->getScalarSizeInBits() == sourceType->getScalarSizeInBits() ||
           (baseT->isPointerTy() &&
            baseT->getPointerElementType()->isFunctionTy()))) {
        if (CheckAllocaUsesInternal(pBitCast))
          continue;
      }
      // Not a candidate.
      return false;
    } else if (PtrToIntInst *PTI = dyn_cast<PtrToIntInst>(*use_it)) {
      if (CheckPtrToIntCandidate(PTI))
        continue;
      // Not a candidate.
      return false;
    } else if (IntrinsicInst *intr = dyn_cast<IntrinsicInst>(*use_it)) {
      auto IID = vc::getAnyIntrinsicID(intr);
      if (IID == llvm::Intrinsic::lifetime_start ||
          IID == llvm::Intrinsic::lifetime_end ||
          IID == GenXIntrinsic::genx_gather_private ||
          IID == GenXIntrinsic::genx_scatter_private ||
          IID == llvm::Intrinsic::masked_gather ||
          IID == llvm::Intrinsic::masked_scatter) {
        continue;
      }
      return false;
    } else if (PHINode *phi = dyn_cast<PHINode>(*use_it)) {
      // Only GEPs with same base and bitcasts with same src yet supported
      Value *pPtrOp = nullptr;
      if (auto BC = dyn_cast<BitCastInst>(I))
        pPtrOp = BC->getOperand(0);
      else if (auto GEP = dyn_cast<GetElementPtrInst>(I))
        pPtrOp = GEP->getPointerOperand();
      else
        return false;

      if (all_of(phi->incoming_values(), [&](Value *V) {
            if (auto GEP = dyn_cast<GetElementPtrInst>(V))
              return GEP->getPointerOperand() == pPtrOp;
            else if (auto BC = dyn_cast<BitCastInst>(V))
              return BC->getOperand(0) == pPtrOp;
            return false;
          }))
        if (CheckAllocaUsesInternal(phi))
          continue;
      // Not a candidate.
      return false;
    } else {
      // This is some other instruction. Right now we don't want to handle these
      return false;
    }
  }
  return true;
}

bool TransformPrivMem::CheckIfAllocaPromotable(AllocaInst &Alloca) {
  // Cannot promote VLA.
  auto MaybeSize = Alloca.getAllocationSizeInBits(*m_pDL);
  if (!MaybeSize.hasValue())
    return false;
  auto AllocaSize = MaybeSize.getValue() / genx::ByteBits;
  if (!ForcePromotion && AllocaSize > SingleAllocaLimitOpt.getValue()) {
    LargeAllocasWereLeft = true;
    return false;
  }

  // Don't even look at non-array or non-struct allocas.
  // (extractAllocaDim can not handle them anyway, causing a crash)
  Type *pType = Alloca.getAllocatedType();
  if ((!pType->isStructTy() && !pType->isArrayTy() && !pType->isVectorTy()) ||
      Alloca.isArrayAllocation())
    return false;

  Type *baseType = GetBaseType(pType, nullptr);
  if (baseType == nullptr)
    return false;
  auto Ty = baseType->getScalarType();
  // only handle case with a simple base type
  if (!(Ty->isFloatingPointTy() || Ty->isIntegerTy()) &&
      !(Ty->isPointerTy() && Ty->getPointerElementType()->isFunctionTy()))
    return false;

  // After promotion the variable will be illegal.
  auto &VecTy = getVectorTypeForAlloca(Alloca, *Ty, *m_pDL);
  if (!visa::Variable::isLegal(VecTy, *m_pDL))
    return false;

  return CheckAllocaUsesInternal(&Alloca);
}

void TransformPrivMem::visitStore(StoreInst &I) {
  if (I.getValueOperand()->getType()->isAggregateType())
    m_StoresToHandle.push(&I);
}

void TransformPrivMem::visitAllocaInst(AllocaInst &I) {
  // find those allocas that can be promoted as a whole-vector
  if (CheckIfAllocaPromotable(I))
    m_allocasToPrivMem.push_back(&I);
}

void TransformPrivMem::selectAllocasToHandle() {
  if (m_allocasToPrivMem.empty())
    return;
  // Promote them all.
  if (ForcePromotion)
    return;

  std::sort(m_allocasToPrivMem.begin(), m_allocasToPrivMem.end(),
            [this](const AllocaInst *LHS, const AllocaInst *RHS) {
              return LHS->getAllocationSizeInBits(*m_pDL).getValue() <
                     RHS->getAllocationSizeInBits(*m_pDL).getValue();
            });
  auto LastIt = vc::upper_partial_sum_bound(
      m_allocasToPrivMem.begin(), m_allocasToPrivMem.end(),
      TotalAllocaLimitOpt.getValue(),
      [this](std::size_t PrevSum, const AllocaInst *CurAlloca) {
        return PrevSum + CurAlloca->getAllocationSizeInBits(*m_pDL).getValue() /
                             genx::ByteBits;
      });

  // if alloca size exceeds alloc size threshold, emit warning
  // and discard promotion
  if (LastIt != m_allocasToPrivMem.end())
    LargeAllocasWereLeft = true;
  m_allocasToPrivMem.erase(LastIt, m_allocasToPrivMem.end());
}

void TransformPrivMem::handleAllocaInst(llvm::AllocaInst *pAlloca) {
  // Extract the Alloca size and the base Type
  Type *pType = pAlloca->getType()->getPointerElementType();
  Type *pBaseType = GetBaseType(pType, nullptr);
  if (!pBaseType)
    return;
  pBaseType = pBaseType->getScalarType();
  llvm::AllocaInst *pVecAlloca = createVectorForAlloca(pAlloca, pBaseType);
  if (!pVecAlloca)
    return;
  // skip processing of allocas that are already fine
  if (pVecAlloca->getType() == pAlloca->getType())
    return;

  IRBuilder<> IRB(pVecAlloca);
  GenericVectorIndex StartIdx{
      IRB.getInt32(0), static_cast<int>(m_pDL->getTypeSizeInBits(pBaseType))};
  TransposeHelperPromote helper(pVecAlloca, m_pDL);
  helper.handleAllocaSources(*pAlloca, StartIdx);
  helper.EraseDeadCode();
}

void TransposeHelper::EraseDeadCode() {
  for (Instruction *I : m_toBeRemoved)
    I->dropAllReferences();
  for (Instruction *I : m_toBeRemoved)
    I->eraseFromParent();
}

void TransposeHelper::handleBCInst(BitCastInst &BC, GenericVectorIndex Idx) {
  m_toBeRemoved.push_back(&BC);
  Type *DstDerefTy = GetBaseType(
      BC.getType()->getScalarType()->getPointerElementType(), nullptr);
  Type *SrcDerefTy = GetBaseType(
      BC.getOperand(0)->getType()->getScalarType()->getPointerElementType(),
      nullptr);
  IGC_ASSERT(DstDerefTy);
  IGC_ASSERT(SrcDerefTy);
  // either the point-to-element-type is the same or
  // the point-to-element-type is the byte
  if (DstDerefTy->getScalarSizeInBits() == SrcDerefTy->getScalarSizeInBits() ||
      (DstDerefTy->isPointerTy() &&
       DstDerefTy->getPointerElementType()->isFunctionTy())) {
    handleAllocaSources(BC, Idx);
    return;
  }

  IGC_ASSERT(DstDerefTy->getScalarSizeInBits() == 8);
  IRBuilder<> IRB(&BC);
  auto ElementSize =
      SrcDerefTy->getScalarSizeInBits() / DstDerefTy->getScalarSizeInBits();
  Value *Scale = nullptr;
  if (Idx.Index->getType()->isVectorTy()) {
    auto Width =
        cast<IGCLLVM::FixedVectorType>(Idx.Index->getType())->getNumElements();
    Scale = ConstantVector::getSplat(IGCLLVM::getElementCount(Width),
                                     IRB.getInt32(ElementSize));
  } else
    Scale = IRB.getInt32(ElementSize);
  auto NewIdx = IRB.CreateMul(Idx.Index, Scale);
  handleAllocaSources(
      BC, {NewIdx, static_cast<int>(DstDerefTy->getScalarSizeInBits())});
}

void TransposeHelper::handlePTIInst(PtrToIntInst &PTI, GenericVectorIndex Idx) {
  IGC_ASSERT(PTI.hasOneUse());
  IGC_ASSERT(isa<InsertElementInst>(PTI.user_back()));
  IRBuilder<> IRB(&PTI);
  auto *Insert = PTI.user_back();
  auto *CastedIdx = IRB.CreateZExt(Idx.Index, PTI.getType(), PTI.getName());
  auto *Mul = IRB.CreateMul(
      CastedIdx,
      ConstantInt::get(CastedIdx->getType(), Idx.getElementSizeInBytes()), "");
  PTI.replaceAllUsesWith(Mul);
  PTI.eraseFromParent();
  IGC_ASSERT(Insert->hasOneUse());
  IGC_ASSERT(isa<ShuffleVectorInst>(Insert->user_back()));
  auto *Shuffle = Insert->user_back();
  IGC_ASSERT(Shuffle->hasOneUse());
  IGC_ASSERT(isa<BinaryOperator>(Shuffle->user_back()));
  handleAllocaSources(*(Shuffle->user_back()),
                      {Shuffle->user_back(), Idx.ElementSizeInBits});
}

void TransposeHelper::handleAllocaSources(Instruction &Inst,
                                          GenericVectorIndex Idx) {
  SmallVector<Value *, 10> Users{Inst.user_begin(), Inst.user_end()};

  for (auto *User : Users) {
    if (GetElementPtrInst *pGEP = dyn_cast<GetElementPtrInst>(User)) {
      handleGEPInst(pGEP, Idx);
    } else if (BitCastInst *BC = dyn_cast<BitCastInst>(User)) {
      handleBCInst(*BC, Idx);
    } else if (PtrToIntInst *PTI = dyn_cast<PtrToIntInst>(User)) {
      handlePTIInst(*PTI, Idx);
    } else if (StoreInst *pStore = llvm::dyn_cast<StoreInst>(User)) {
      handleStoreInst(pStore, Idx);
    } else if (LoadInst *pLoad = llvm::dyn_cast<LoadInst>(User)) {
      handleLoadInst(pLoad, Idx.Index);
    } else if (PHINode *pPhi = llvm::dyn_cast<PHINode>(User)) {
      handlePHINode(pPhi, Idx, Inst.getParent());
    } else if (IntrinsicInst *IntrInst = dyn_cast<IntrinsicInst>(User)) {
      auto IID = vc::getAnyIntrinsicID(IntrInst);
      if (IID == llvm::Intrinsic::lifetime_start ||
          IID == llvm::Intrinsic::lifetime_end)
        IntrInst->eraseFromParent();
      else if (IID == GenXIntrinsic::genx_gather_private)
        handlePrivateGather(IntrInst, Idx.Index);
      else if (IID == GenXIntrinsic::genx_scatter_private)
        handlePrivateScatter(IntrInst, Idx.Index);
      else if (IID == GenXIntrinsic::genx_svm_gather)
        handleSVMGather(IntrInst, Idx.Index);
      else if (IID == GenXIntrinsic::genx_svm_scatter)
        handleSVMScatter(IntrInst, Idx.Index);
      else if (IntrInst->getIntrinsicID() == llvm::Intrinsic::masked_gather)
        handleLLVMGather(IntrInst, Idx.Index);
      else if (IntrInst->getIntrinsicID() == llvm::Intrinsic::masked_scatter)
        handleLLVMScatter(IntrInst, Idx.Index);
    }
  }
}

void TransposeHelper::handleGEPInst(GetElementPtrInst *GEP,
                                    GenericVectorIndex Idx) {
  m_toBeRemoved.push_back(GEP);
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
          IGC_ASSERT_MESSAGE(IdxWidth == Width, "GEP has inconsistent vector-index width");
      }
    }
  }
  Type *Ty = PtrTy;
  gep_type_iterator GTI = gep_type_begin(GEP);
  IRBuilder<> IRB(GEP);
  Value *pScalarizedIdx =
      (IdxWidth == 1)
          ? IRB.getInt32(0)
          : ConstantVector::getSplat(IGCLLVM::getElementCount(IdxWidth),
                                     IRB.getInt32(0));
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *GEPIdx = *OI;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      auto Field = cast<ConstantInt>(GEPIdx)->getZExtValue();
      if (Field) {
        int Offset = m_pDL->getStructLayout(StTy)->getElementOffset(Field);
        IGC_ASSERT(Idx.getElementSizeInBytes());
        IGC_ASSERT_MESSAGE(Offset % Idx.getElementSizeInBytes() == 0,
            "the offset must be a multiple of the current vector granulation");
        Constant *OffsetVal =
            IRB.getInt32(Offset / Idx.getElementSizeInBytes());
        if (IdxWidth > 1)
          OffsetVal = ConstantVector::getSplat(
              IGCLLVM::getElementCount(IdxWidth), OffsetVal);
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, OffsetVal);
      }
      Ty = StTy->getElementType(Field);
    } else {
      Ty = GTI.getIndexedType();
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(GEPIdx)) {
        if (!CI->isZero()) {
          Constant *OffsetVal =
              IRB.getInt32(m_pDL->getTypeAllocSize(Ty) * CI->getZExtValue() /
                           Idx.getElementSizeInBytes());
          if (IdxWidth > 1)
            OffsetVal = ConstantVector::getSplat(
                IGCLLVM::getElementCount(IdxWidth), OffsetVal);
          pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, OffsetVal);
        }
      } else if (!GEPIdx->getType()->isVectorTy() && IdxWidth <= 1) {
        Value *NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, IRB.getInt32Ty());
        IGC_ASSERT(Idx.getElementSizeInBytes());
        IGC_ASSERT_MESSAGE(m_pDL->getTypeAllocSize(Ty) % Idx.getElementSizeInBytes() == 0,
            "current type size must be multiple of current offset granulation "
            "to be represented in this offset");
        auto ElementSize =
            m_pDL->getTypeAllocSize(Ty) / Idx.getElementSizeInBytes();
        NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, NewIdx);
      } else {
        // the input idx is a vector or the one of the GEP index is vector
        Value * NewIdx = nullptr;
        IGC_ASSERT_MESSAGE(m_pDL->getTypeAllocSize(Ty) % Idx.getElementSizeInBytes() == 0,
            "current type size must be multiple of current offset granulation "
            "to be represented in this offset");
        auto ElementSize =
            m_pDL->getTypeAllocSize(Ty) / Idx.getElementSizeInBytes();
        if (GEPIdx->getType()->isVectorTy()) {
          IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(GEPIdx->getType())
                         ->getNumElements() == IdxWidth);
          NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, pScalarizedIdx->getType());
          NewIdx = IRB.CreateMul(NewIdx, ConstantVector::getSplat(
                                             IGCLLVM::getElementCount(IdxWidth),
                                             IRB.getInt32(ElementSize)));
        } else {
          NewIdx = IRB.CreateZExtOrTrunc(GEPIdx, IRB.getInt32Ty());
          // splat the new-idx into a vector
          NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
        }
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, NewIdx);
      }
    }
  }
  if (!Idx.Index->getType()->isVectorTy() && IdxWidth <= 1) {
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, Idx.Index);
  } else if (Idx.Index->getType()->isVectorTy()) {
    IGC_ASSERT(cast<IGCLLVM::FixedVectorType>(Idx.Index->getType())
                   ->getNumElements() == IdxWidth);
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, Idx.Index);
  } else {
    auto SplatIdx = IRB.CreateVectorSplat(IdxWidth, Idx.Index);
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, SplatIdx);
  }
  handleAllocaSources(*GEP, {pScalarizedIdx, Idx.ElementSizeInBits});
}

// Pass acummulated idx through new phi
void TransposeHelper::handlePHINode(PHINode *pPhi, GenericVectorIndex Idx,
                                    BasicBlock *pIncomingBB) {
  PHINode *NewPhi = nullptr;
  // If phi is not yet visited
  if (!m_phiReplacement.count(pPhi)) {
    IRBuilder<> IRB(pPhi);
    NewPhi = IRB.CreatePHI(Idx.Index->getType(), pPhi->getNumIncomingValues(),
                           "idx");
    m_phiReplacement.insert(std::make_pair(pPhi, NewPhi));
    m_toBeRemoved.push_back(pPhi);
  } else
    NewPhi = m_phiReplacement[pPhi];
  NewPhi->addIncoming(Idx.Index, pIncomingBB);
  handleAllocaSources(*pPhi, {NewPhi, Idx.ElementSizeInBits});
}

// Loads vector and casts it if necessary.
// \p CastTo describes vector element type to cast to.
template <typename FolderT = ConstantFolder>
Instruction *loadAndCastVector(AllocaInst &VecAlloca, Type &CastTo,
                               IRBuilder<FolderT> &IRB) {
  auto *LoadVecAlloca =
      IRB.CreateLoad(VecAlloca.getType()->getPointerElementType(), &VecAlloca);
  auto *AllocatedElemTy = LoadVecAlloca->getType()->getScalarType();
  bool IsFuncPointer =
      CastTo.isPointerTy() && CastTo.getPointerElementType()->isFunctionTy();
  if (AllocatedElemTy == &CastTo || IsFuncPointer)
    return LoadVecAlloca;
  auto AllocatedWidth = cast<IGCLLVM::FixedVectorType>(LoadVecAlloca->getType())
                            ->getNumElements();
  IGC_ASSERT(AllocatedElemTy->getScalarSizeInBits() >=
             CastTo.getScalarSizeInBits());
  IGC_ASSERT(CastTo.getScalarSizeInBits());
  IGC_ASSERT((AllocatedElemTy->getScalarSizeInBits() %
              CastTo.getScalarSizeInBits()) == 0);
  auto CastedWidth = AllocatedWidth * (AllocatedElemTy->getScalarSizeInBits() /
                                       CastTo.getScalarSizeInBits());
  return cast<Instruction>(IRB.CreateBitCast(
      LoadVecAlloca, IGCLLVM::FixedVectorType::get(&CastTo, CastedWidth),
      "post.load.bc"));
}

// Casts \p NewValue if its type doesn't correspond to allocated vector type,
// then stores the value.
template <typename FolderT = ConstantFolder>
Instruction *castAndStoreVector(AllocaInst &VecAlloca, Value &NewValue,
                                IRBuilder<FolderT> &IRB) {
  auto *CastedValue = &NewValue;
  if (VecAlloca.getAllocatedType() != NewValue.getType())
    CastedValue = IRB.CreateBitCast(&NewValue, VecAlloca.getAllocatedType(),
                                    NewValue.getName() + ".pre.store.bc");
  return IRB.CreateStore(CastedValue, &VecAlloca);
}

void TransposeHelperPromote::handleLoadInst(LoadInst *pLoad,
                                            Value *pScalarizedIdx) {
  IGC_ASSERT(pLoad->isSimple());
  IRBuilder<> IRB(pLoad);
  auto LdTy = pLoad->getType()->getScalarType();
  auto *ReadIn = loadAndCastVector(*pVecAlloca, *LdTy, IRB);
  bool IsFuncPointer = pLoad->getPointerOperandType()->isPointerTy() &&
    pLoad->getPointerOperandType()->getPointerElementType()->isPointerTy() &&
    pLoad->getPointerOperandType()->getPointerElementType()->getPointerElementType()->isFunctionTy();
  if (IsFuncPointer) {
    Region R(
        IGCLLVM::FixedVectorType::get(
            cast<VectorType>(pVecAlloca->getType()->getPointerElementType())
                ->getElementType(),
            m_pDL->getTypeSizeInBits(LdTy) /
                m_pDL->getTypeSizeInBits(
                    cast<VectorType>(
                        pVecAlloca->getType()->getPointerElementType())
                        ->getElementType())),
        m_pDL);
    if (!pScalarizedIdx->getType()->isIntegerTy(16)) {
      pScalarizedIdx = IRB.CreateZExtOrTrunc(pScalarizedIdx, Type::getInt16Ty(pLoad->getContext()));
    }
    R.Indirect = pScalarizedIdx;
    auto *Result = R.createRdRegion(ReadIn, pLoad->getName(), pLoad,
                                    pLoad->getDebugLoc(), true);
    if (!Result->getType()->isPointerTy()) {
      auto *BC =
          IRB.CreateBitCast(Result, Type::getInt64Ty(pLoad->getContext()));
      auto *PtrToI = IRB.CreateIntToPtr(BC, pLoad->getType(), pLoad->getName());
      pLoad->replaceAllUsesWith(PtrToI);
    } else
      pLoad->replaceAllUsesWith(Result);
  }
  else if (pLoad->getType()->isVectorTy()) {
    // A vector load
    // %v = load <2 x float>* %ptr
    // becomes
    // %w = load <32 x float>* %ptr1
    // %v0 = extractelement <32 x float> %w, i32 %idx
    // %v1 = extractelement <32 x float> %w, i32 %idx+1
    // replace all uses of %v with <%v0, %v1>
    auto Len =
        cast<IGCLLVM::FixedVectorType>(pLoad->getType())->getNumElements();
    Value *Result = UndefValue::get(pLoad->getType());
    for (unsigned i = 0; i < Len; ++i) {
      Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
      auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
      auto Val = IRB.CreateExtractElement(ReadIn, Idx);
      Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
    }
    pLoad->replaceAllUsesWith(Result);
  } else {
    auto Result = IRB.CreateExtractElement(ReadIn, pScalarizedIdx);
    pLoad->replaceAllUsesWith(Result);
  }
  pLoad->eraseFromParent();
}

void TransposeHelperPromote::handleStoreInst(StoreInst *pStore,
                                             GenericVectorIndex ScalarizedIdx) {
  // Add Store instruction to remove list
  IGC_ASSERT(pStore->isSimple());
  IRBuilder<> IRB(pStore);
  llvm::Value *pStoreVal = pStore->getValueOperand();
  auto *StTy = pStoreVal->getType()->getScalarType();
  Value *WriteOut = loadAndCastVector(*pVecAlloca, *StTy, IRB);

  bool IsFuncPointerStore =
      (isFuncPointerVec(pStoreVal) ||
       (pStoreVal->getType()->isPointerTy() &&
        pStoreVal->getType()->getPointerElementType()->isFunctionTy()));
  if (IsFuncPointerStore) {
    auto *NewStoreVal = pStoreVal;
    IGC_ASSERT(cast<VectorType>(pVecAlloca->getType()->getPointerElementType())->getElementType()->isIntegerTy(64));
    if (NewStoreVal->getType()->isPointerTy() &&
        NewStoreVal->getType()->getPointerElementType()->isFunctionTy()) {
      NewStoreVal = IRB.CreatePtrToInt(
          NewStoreVal, IntegerType::getInt64Ty(pStore->getContext()));
    }
    Region R(NewStoreVal, m_pDL);
    if (!ScalarizedIdx.Index->getType()->isIntegerTy(16)) {
      ScalarizedIdx.Index = IRB.CreateZExtOrTrunc(
          ScalarizedIdx.Index, Type::getInt16Ty(pStore->getContext()));
    }
    if (auto *ConstIdx = dyn_cast<llvm::Constant>(ScalarizedIdx.Index))
      R.Indirect = ConstantExpr::getMul(
          ConstIdx,
          ConstantInt::get(IRB.getInt16Ty(),
                           m_pDL->getTypeSizeInBits(
                               NewStoreVal->getType()->getScalarType()) /
                               genx::ByteBits));
    else
      R.Indirect = ScalarizedIdx.Index;
    WriteOut =
        R.createWrRegion(WriteOut, NewStoreVal, pStore->getName() + ".promoted",
                         pStore, pStore->getDebugLoc());
  } else if (pStoreVal->getType()->isVectorTy()) {
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
        cast<IGCLLVM::FixedVectorType>(pStoreVal->getType())->getNumElements();
    for (unsigned i = 0; i < Len; ++i) {
      Value *VectorIdx = ConstantInt::get(ScalarizedIdx.Index->getType(), i);
      auto *Val = IRB.CreateExtractElement(pStoreVal, VectorIdx);
      auto *Idx = IRB.CreateAdd(ScalarizedIdx.Index, VectorIdx);
      IGC_ASSERT_MESSAGE(m_pDL->getTypeSizeInBits(Val->getType()) == ScalarizedIdx.ElementSizeInBits,
          "stored type considered vector element size must correspond");
      WriteOut = IRB.CreateInsertElement(WriteOut, Val, Idx);
    }
  } else {
    IGC_ASSERT_MESSAGE(m_pDL->getTypeSizeInBits(pStoreVal->getType()) == ScalarizedIdx.ElementSizeInBits,
        "stored type considered vector element size must correspond");
    WriteOut =
        IRB.CreateInsertElement(WriteOut, pStoreVal, ScalarizedIdx.Index);
  }
  castAndStoreVector(*pVecAlloca, *WriteOut, IRB);
  pStore->eraseFromParent();
}

void TransposeHelperPromote::handlePrivateGather(IntrinsicInst *pInst,
                                          Value *pScalarizedIdx) {
  IRBuilder<> IRB(pInst);
  IGC_ASSERT(pInst->getType()->isVectorTy());
  Value *pLoadVecAlloca = IRB.CreateLoad(
      pVecAlloca->getType()->getPointerElementType(), pVecAlloca);
  auto *InstTy = cast<IGCLLVM::FixedVectorType>(pInst->getType());
  auto N = InstTy->getNumElements();
  auto ElemType = InstTy->getElementType();

  // A vector load
  // %v = <2 x float> gather %pred, %ptr, %offset, %old_value
  // becomes
  // %w = load <32 x float>* %ptr1
  // %v0 = <2 x float> rdregion <32 x float> %w, i32 %offsets, %stride
  //
  // replace all uses of %v with <%v0, %v1>
  Region R(pInst);
  int64_t v0 = 0;
  int64_t diff = 0;
  ConstantInt *CI = dyn_cast<ConstantInt>(pScalarizedIdx);
  PointerType *GatherPtrTy =
      dyn_cast<PointerType>(pInst->getArgOperand(1)->getType());
  // pScalarizedIdx is an indice of element, so
  // count byte offset depending on the type of pointer in gather
  IGC_ASSERT(GatherPtrTy);
  unsigned GatherPtrNumBytes =
      GatherPtrTy->getPointerElementType()->getPrimitiveSizeInBits() / 8;
  if (CI != nullptr &&
      IsLinearVectorConstantInts(pInst->getArgOperand(2), v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    int BytesOffset = CI->getSExtValue() * GatherPtrNumBytes;
    R.Offset = v0 + BytesOffset;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  } else {
    auto OffsetType = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pInst->getArgOperand(2), OffsetType, true);
    auto Cast = IRB.CreateIntCast(
        pScalarizedIdx, IntegerType::getInt16Ty(pInst->getContext()), true);
    auto Scale = IRB.CreateMul(IRB.getInt16(GatherPtrNumBytes), Cast);
    auto vec = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), 1);
    auto GEPOffsets =
        IRB.CreateInsertElement(UndefValue::get(vec), Scale, IRB.getInt32(0));
    GEPOffsets = IRB.CreateShuffleVector(
        GEPOffsets, UndefValue::get(vec),
        ConstantAggregateZero::get(IGCLLVM::FixedVectorType::get(
            IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateAdd(GEPOffsets, Offsets);
    R.Indirect = Offsets;
    R.Width = 1;
    R.Stride = 0;
    R.VStride = 0;
  }
  Value *Result =
      R.createRdRegion(pLoadVecAlloca, pInst->getName(), pInst /*InsertBefore*/,
                       pInst->getDebugLoc(), true /*AllowScalar*/);

  // if old-value is not undefined and predicate is not all-one,
  // create a select  auto OldVal = pInst->getArgOperand(3);
  auto PredVal = pInst->getArgOperand(0);
  bool PredAllOne = false;
  if (auto C = dyn_cast<ConstantVector>(PredVal)) {
    if (auto B = C->getSplatValue())
      PredAllOne = B->isOneValue();
  }
  auto OldVal = pInst->getArgOperand(3);
  if (!PredAllOne && !isa<UndefValue>(OldVal)) {
    Result = IRB.CreateSelect(PredVal, Result, OldVal);
  }

  pInst->replaceAllUsesWith(Result);
  pInst->eraseFromParent();
}

void TransposeHelperPromote::handlePrivateScatter(llvm::IntrinsicInst *pInst,
                                           llvm::Value *pScalarizedIdx) {
  // Add Store instruction to remove list
  IRBuilder<> IRB(pInst);
  llvm::Value *pStoreVal = pInst->getArgOperand(3);
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(
      pVecAlloca->getType()->getPointerElementType(), pVecAlloca);
  IGC_ASSERT(pStoreVal->getType()->isVectorTy());
  auto *StoreValTy = cast<IGCLLVM::FixedVectorType>(pStoreVal->getType());
  auto N = StoreValTy->getNumElements();
  auto ElemType = StoreValTy->getElementType();
  // A vector scatter
  // scatter %pred, %ptr, %offset, %newvalue
  // becomes
  // %w = load <32 x float> *%ptr1
  // %w1 = <32 x float> wrregion %w, newvalue, %offset, %pred
  // store <32 x float> %w1, <32 x float>* %ptr1

  // Create the new wrregion
  Region R(pStoreVal);
  int64_t v0 = 0;
  int64_t diff = 0;
  ConstantInt *CI = dyn_cast<ConstantInt>(pScalarizedIdx);
  PointerType *ScatterPtrTy =
      dyn_cast<PointerType>(pInst->getArgOperand(1)->getType());
  // pScalarizedIdx is an indice of element, so
  // count byte offset depending on the type of pointer in scatter
  IGC_ASSERT(ScatterPtrTy);
  unsigned ScatterPtrNumBytes =
      ScatterPtrTy->getPointerElementType()->getPrimitiveSizeInBits() / 8;
  if (CI != nullptr && IsLinearVectorConstantInts(pInst->getArgOperand(2), v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    int BytesOffset = CI->getSExtValue() * ScatterPtrNumBytes;
    R.Offset = v0 + BytesOffset;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  } else {
    auto OffsetType = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pInst->getArgOperand(2), OffsetType, true);
    auto Cast = IRB.CreateIntCast(
        pScalarizedIdx, IntegerType::getInt16Ty(pInst->getContext()), true);
    auto Scale = IRB.CreateMul(IRB.getInt16(ScatterPtrNumBytes), Cast);
    auto vec = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), 1);
    auto GEPOffsets =
        IRB.CreateInsertElement(UndefValue::get(vec), Scale, IRB.getInt32(0));
    GEPOffsets = IRB.CreateShuffleVector(
        GEPOffsets, UndefValue::get(vec),
        ConstantAggregateZero::get(IGCLLVM::FixedVectorType::get(
            IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateAdd(GEPOffsets, Offsets);
    R.Indirect = Offsets;
    R.Width = 1;
    R.Stride = 0;
    R.VStride = 0;
  }
  R.Mask = pInst->getArgOperand(0);
  auto NewInst = cast<Instruction>(
      R.createWrRegion(pLoadVecAlloca, pStoreVal, pInst->getName(),
                       pInst /*InsertBefore*/, pInst->getDebugLoc()));

  IRB.CreateStore(NewInst, pVecAlloca);
  pInst->eraseFromParent();
}

void TransposeHelperPromote::handleLLVMGather(IntrinsicInst *pInst,
  Value *pScalarizedIdx) {
  IRBuilder<> IRB(pInst);
  auto *InstTy = cast<IGCLLVM::FixedVectorType>(pInst->getType());
  auto N = InstTy->getNumElements();
  auto ElemType = InstTy->getElementType();
  Value *LoadVecAlloca = loadAndCastVector(*pVecAlloca, *ElemType, IRB);

  // A vector load
  // %v = <2 x float> gather %pred, %vector_of_ptr, %old_value
  // becomes
  // %w = load <32 x float>* %ptr1
  // %v0 = <2 x float> rdregion <32 x float> %w, i32 %offsets, %stride
  //
  // replace all uses of %v with <%v0, %v1>
  Region R(pInst);
  int64_t v0 = 0;
  int64_t diff = 0;
  // count byte offset depending on the type of pointer in gather
  unsigned ElemNumBytes = ElemType->getPrimitiveSizeInBits() / 8;
  if (IsLinearVectorConstantInts(pScalarizedIdx, v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    R.Offset = v0;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  }
  else {
    auto OffsetType = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pScalarizedIdx, OffsetType, false);
    auto ScaleVec =
      IRB.CreateInsertElement(UndefValue::get(OffsetType), IRB.getInt16(ElemNumBytes), IRB.getInt32(0));
    ScaleVec = IRB.CreateShuffleVector(
        ScaleVec, UndefValue::get(OffsetType),
        ConstantAggregateZero::get(IGCLLVM::FixedVectorType::get(
            IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateMul(Offsets, ScaleVec);
    R.Indirect = Offsets;
    R.Width = 1;
    R.Stride = 0;
    R.VStride = 0;
  }
  Value *Result =
      R.createRdRegion(LoadVecAlloca, pInst->getName(), pInst /*InsertBefore*/,
                       pInst->getDebugLoc(), true /*AllowScalar*/);

  // if old-value is not undefined and predicate is not all-one,
  // create a select  auto OldVal = pInst->getArgOperand(3);
  auto PredVal = pInst->getArgOperand(2);
  bool PredAllOne = false;
  if (auto C = dyn_cast<ConstantVector>(PredVal)) {
    if (auto B = C->getSplatValue())
      PredAllOne = B->isOneValue();
  }
  auto OldVal = pInst->getArgOperand(3);
  if (!PredAllOne && !isa<UndefValue>(OldVal)) {
    Result = IRB.CreateSelect(PredVal, Result, OldVal);
  }

  pInst->replaceAllUsesWith(Result);
  pInst->eraseFromParent();
}

void TransposeHelperPromote::handleLLVMScatter(llvm::IntrinsicInst *pInst,
  llvm::Value *pScalarizedIdx) {
  // Add Store instruction to remove list
  IRBuilder<> IRB(pInst);
  Value *StoredValue = pInst->getArgOperand(0);
  IGC_ASSERT(StoredValue->getType()->isVectorTy());
  auto N =
      cast<IGCLLVM::FixedVectorType>(StoredValue->getType())->getNumElements();
  auto *ElemType =
      cast<IGCLLVM::FixedVectorType>(StoredValue->getType())->getElementType();
  Value *LoadVecAlloca = loadAndCastVector(*pVecAlloca, *ElemType, IRB);
  // A vector scatter
  // scatter %newvalue, %ptr, i32 align, %pred
  // becomes
  // %w = load %vec.alloca
  // %w1 = wrregion %w, %newvalue, %indexed.ptr, %pred
  // store %w1, %vec.alloca

  // Create the new wrregion
  Region R{StoredValue};
  int64_t v0 = 0;
  int64_t diff = 0;
  // pScalarizedIdx is an indice of element, so
  // count byte offset depending on the type of pointer in scatter
  unsigned ElemNumBytes = ElemType->getPrimitiveSizeInBits() / 8;
  if (IsLinearVectorConstantInts(pScalarizedIdx, v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    R.Offset = v0;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  }
  else {
    auto OffsetType = IGCLLVM::FixedVectorType::get(
        IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pScalarizedIdx, OffsetType, false);
    auto ScaleVec = IRB.CreateInsertElement(UndefValue::get(OffsetType),
      IRB.getInt16(ElemNumBytes),
      IRB.getInt32(0));
    ScaleVec = IRB.CreateShuffleVector(
        ScaleVec, UndefValue::get(OffsetType),
        ConstantAggregateZero::get(IGCLLVM::FixedVectorType::get(
            IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateMul(Offsets, ScaleVec);
    R.Indirect = Offsets;
    R.Width = 1;
    R.Stride = 0;
    R.VStride = 0;
  }
  R.Mask = pInst->getArgOperand(3);
  auto NewInst = cast<Instruction>(
      R.createWrRegion(LoadVecAlloca, StoredValue, pInst->getName(),
                       pInst /*InsertBefore*/, pInst->getDebugLoc()));

  castAndStoreVector(*pVecAlloca, *NewInst, IRB);
  pInst->eraseFromParent();
}

void TransposeHelperPromote::handleSVMGather(IntrinsicInst *pInst,
                                             Value *pScalarizedIdx) {
  // %v = svm_gather %pred, %ptr + %offset
  // is turned into
  // %v0 = load <32 x float> *%ptr1
  // %v1 = <32 x float> rdregion %v0, %offset, %pred

  // here we rely on offset being previously generated
  // by e.g. ISPC

  // part of this is taken from handleLLVMGather above
  IRBuilder<> IRB(pInst);
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(
      pVecAlloca->getType()->getPointerElementType(), pVecAlloca);
  Region R(pInst);
  R.Mask = pInst->getArgOperand(0);
  R.Indirect = IRB.CreateTrunc(
      pScalarizedIdx,
      IGCLLVM::FixedVectorType::get(
          IntegerType::getInt16Ty(pInst->getContext()),
          cast<IGCLLVM::FixedVectorType>(pScalarizedIdx->getType())
              ->getNumElements()),
      "");
  R.Width = 1;
  R.Stride = 0;
  R.VStride = 0;
  Value *Result =
      R.createRdRegion(pLoadVecAlloca, pInst->getName(), pInst /*InsertBefore*/,
                       pInst->getDebugLoc(), true /*AllowScalar*/);
  // if old-value is not undefined and predicate is not all-one,
  // create a select
  auto PredVal = pInst->getArgOperand(2);
  bool PredAllOne = false;
  if (auto C = dyn_cast<ConstantVector>(PredVal)) {
    if (auto B = C->getSplatValue())
      PredAllOne = B->isOneValue();
  }
  auto OldVal = pInst->getArgOperand(3);
  if (!PredAllOne && !isa<UndefValue>(OldVal))
    Result = IRB.CreateSelect(PredVal, Result, OldVal);

  pInst->replaceAllUsesWith(Result);
  pInst->eraseFromParent();
}

void TransposeHelperPromote::handleSVMScatter(IntrinsicInst *pInst,
                                              Value *pScalarizedIdx) {
  IRBuilder<> IRB(pInst);
  Value *pStoreVal = pInst->getArgOperand(3);
  Value *pLoadVecAlloca = IRB.CreateLoad(
      pVecAlloca->getType()->getPointerElementType(), pVecAlloca);
  Region R(pStoreVal);
  R.Mask = pInst->getArgOperand(0);
  R.Indirect = IRB.CreateTrunc(
      pScalarizedIdx,
      IGCLLVM::FixedVectorType::get(
          IntegerType::getInt16Ty(pInst->getContext()),
          cast<IGCLLVM::FixedVectorType>(pScalarizedIdx->getType())
              ->getNumElements()),
      "");
  R.Width = 1;
  R.Stride = 0;
  R.VStride = 0;
  auto NewInst = R.createWrRegion(pLoadVecAlloca, pStoreVal, pInst->getName(),
                                  pInst, pInst->getDebugLoc());
  IRB.CreateStore(NewInst, pVecAlloca);
  pInst->eraseFromParent();
}

} // namespace
