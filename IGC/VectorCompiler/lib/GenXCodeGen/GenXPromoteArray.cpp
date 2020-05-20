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
#include "GenXRegion.h"
#include "GenXUtil.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/ADT/SmallVector.h"

#include <queue>

#define MAX_ALLOCA_PROMOTE_GRF_NUM 96

using namespace llvm;
using namespace genx;

namespace {

// Diagnostic information for error/warning relating array promotion.
class DiagnosticInfoPromoteArray : public DiagnosticInfo {
private:
  std::string Description;

public:
  // Initialize from description
  DiagnosticInfoPromoteArray(const Twine &Desc,
                             DiagnosticSeverity Severity = DS_Error)
      : DiagnosticInfo(llvm::getNextAvailablePluginDiagnosticKind(), Severity),
        Description(Desc.str()) {}

  void print(DiagnosticPrinter &DP) const override {
    DP << "GenXPromoteArray: " << Description;
  }
};

class TransposeHelper {
public:
  TransposeHelper(bool vectorIndex, const llvm::DataLayout *DL,
                  uint64_t baseTypeAllocSize)
      : m_vectorIndex(vectorIndex), m_pDL(DL),
        m_baseTypeAllocSize(baseTypeAllocSize) {}
  void HandleAllocaSources(llvm::Instruction *v, llvm::Value *idx);
  void handleGEPInst(llvm::GetElementPtrInst *pGEP, llvm::Value *idx);
  void handlePHINode(llvm::PHINode *pPhi, llvm::Value *pScalarizedIdx,
                     llvm::BasicBlock *pIncomingBB);
  virtual void handleLoadInst(llvm::LoadInst *pLoad,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handleStoreInst(llvm::StoreInst *pStore,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handlePrivateGather(llvm::IntrinsicInst *pInst,
                     llvm::Value *pScalarizedIdx) = 0;
  virtual void handlePrivateScatter(llvm::IntrinsicInst *pInst,
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
  uint64_t m_baseTypeAllocSize = 0;
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

  virtual llvm::StringRef getPassName() const override {
    return "TransformPrivMem";
  }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.setPreservesCFG();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

  void visitAllocaInst(llvm::AllocaInst &I);

  void visitStore(llvm::StoreInst &St);

  unsigned int extractAllocaSize(llvm::AllocaInst *pAlloca);

private:
  llvm::AllocaInst *createVectorForAlloca(llvm::AllocaInst *pAlloca,
                                          llvm::Type *pBaseType);
  void handleAllocaInst(llvm::AllocaInst *pAlloca);

  bool CheckIfAllocaPromotable(llvm::AllocaInst *pAlloca);

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
INITIALIZE_PASS_END(TransformPrivMem, PASS_FLAG, PASS_DESCRIPTION,
                    PASS_CFG_ONLY, PASS_ANALYSIS)

char TransformPrivMem::ID = 0;

FunctionPass *llvm::createTransformPrivMemPass() {
  return new TransformPrivMem();
}

namespace {

class TransposeHelperPromote : public TransposeHelper {
public:
  void handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx);
  void handleStoreInst(StoreInst *pStore, Value *pScalarizedIdx);
  void handlePrivateGather(IntrinsicInst *pInst, Value *pScalarizedIdx);
  void handlePrivateScatter(IntrinsicInst *pInst, Value *pScalarizedIdx);
  void handleLLVMGather(IntrinsicInst *pInst, Value *pScalarizedIdx);
  void handleLLVMScatter(IntrinsicInst *pInst, Value *pScalarizedIdx);

  AllocaInst *pVecAlloca;

  TransposeHelperPromote(AllocaInst *pAI, const llvm::DataLayout *DL,
                         uint64_t baseTypeAllocSize)
      : TransposeHelper(false, DL, baseTypeAllocSize) {
    pVecAlloca = pAI;
  }
};

TransformPrivMem::TransformPrivMem() : FunctionPass(ID), m_pFunc(nullptr) {
  initializeTransformPrivMemPass(*PassRegistry::getPassRegistry());
}

llvm::AllocaInst *
TransformPrivMem::createVectorForAlloca(llvm::AllocaInst *pAlloca,
                                        llvm::Type *pBaseType) {
  IRBuilder<> IRB(pAlloca);

  unsigned int totalSize = extractAllocaSize(pAlloca) /
                           (unsigned int)(m_pDL->getTypeAllocSize(pBaseType));

  llvm::VectorType *pVecType = llvm::VectorType::get(pBaseType, totalSize);

  AllocaInst *pAllocaValue = IRB.CreateAlloca(pVecType);
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
    StI->getAlignment(), StI->isVolatile());
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

  assert(StI->isSimple());
  assert(AT || ST);

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
  m_allocasToPrivMem.clear();

  visit(F);

  bool AggrRemoved = false;
  while (!m_StoresToHandle.empty()) {
    StoreInst *StI = m_StoresToHandle.front();
    m_StoresToHandle.pop();
    if (StI->getValueOperand()->getType()->isAggregateType())
      AggrRemoved |= replaceAggregatedStore(StI);
  }

  std::vector<llvm::AllocaInst *> &allocaToHandle = m_allocasToPrivMem;

  for (auto pAlloca : allocaToHandle) {
    handleAllocaInst(pAlloca);
  }

  // Last remove alloca instructions
  for (auto pInst : allocaToHandle) {
    if (pInst->use_empty()) {
      pInst->eraseFromParent();
    }
  }
  // IR changed only if we had alloca instruction to optimize or
  // if aggregated stores were replaced
  return !allocaToHandle.empty() || AggrRemoved;
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
             structElemBaseType->getTypeID() != pBaseType->getTypeID()))
          return nullptr;
        pBaseType = structElemBaseType;
      }
      return pBaseType;
    } else if (pType->isArrayTy()) {
      pType = pType->getArrayElementType();
    } else if (pType->isVectorTy()) {
      pType = pType->getVectorElementType();
    } else {
      assert(0);
    }
  }
  if (pType->isPointerTy() && pType->getPointerElementType()->isFunctionTy())
    pType = IntegerType::getInt8Ty(pType->getContext());
  return pType;
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
      Type *baseT =
          GetBaseType(pBitCast->getType()->getPointerElementType(), nullptr);
      Type *sourceType = GetBaseType(
          pBitCast->getOperand(0)->getType()->getPointerElementType(), nullptr);
      assert(sourceType);
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
    } else if (IntrinsicInst *intr = dyn_cast<IntrinsicInst>(*use_it)) {
      auto IID = GenXIntrinsic::getAnyIntrinsicID(intr);
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

bool TransformPrivMem::CheckIfAllocaPromotable(llvm::AllocaInst *pAlloca) {
  unsigned int allocaSize = extractAllocaSize(pAlloca);
  unsigned int allowedAllocaSizeInBytes = MAX_ALLOCA_PROMOTE_GRF_NUM * 32;

  // if alloca size exceeds alloc size threshold, emit warning
  // and discard promotion
  if (allocaSize > allowedAllocaSizeInBytes) {
    DiagnosticInfoPromoteArray Warn(
        m_pFunc->getName() + " allocation size is too big: using TPM",
        DS_Warning);
    m_pFunc->getContext().diagnose(Warn);
    return false;
  }

  // Don't even look at non-array or non-struct allocas.
  // (extractAllocaDim can not handle them anyway, causing a crash)
  llvm::Type *pType = pAlloca->getAllocatedType();
  if ((!pType->isStructTy() && !pType->isArrayTy() && !pType->isVectorTy()) ||
      pAlloca->isArrayAllocation())
    return false;

  Type *baseType = GetBaseType(pType, nullptr);
  if (baseType == nullptr)
    return false;
  auto Ty = baseType->getScalarType();
  // only handle case with a simple base type
  if (!(Ty->isFloatingPointTy() || Ty->isIntegerTy()) &&
      !(Ty->isPointerTy() && Ty->getPointerElementType()->isFunctionTy()))
    return false;

  return CheckAllocaUsesInternal(pAlloca);
}

void TransformPrivMem::visitStore(StoreInst &I) {
  if (I.getValueOperand()->getType()->isAggregateType())
    m_StoresToHandle.push(&I);
}

void TransformPrivMem::visitAllocaInst(AllocaInst &I) {
  // find those allocas that can be promoted as a whole-vector
  if (!CheckIfAllocaPromotable(&I)) {
    return;
  }
  m_allocasToPrivMem.push_back(&I);
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
  Value *idx = IRB.getInt32(0);
  TransposeHelperPromote helper(pVecAlloca, m_pDL,
                                m_pDL->getTypeAllocSize(pBaseType));
  helper.HandleAllocaSources(pAlloca, idx);
  helper.EraseDeadCode();
}

void TransposeHelper::EraseDeadCode() {
  for (Instruction *I : m_toBeRemoved)
    I->dropAllReferences();
  for (Instruction *I : m_toBeRemoved)
    I->eraseFromParent();
}

void TransposeHelper::HandleAllocaSources(Instruction *v, Value *idx) {
  SmallVector<Value *, 10> instructions;
  for (Value::user_iterator it = v->user_begin(), e = v->user_end(); it != e;
       ++it) {
    Value *inst = cast<Value>(*it);
    instructions.push_back(inst);
  }

  for (auto instruction : instructions) {
    if (GetElementPtrInst *pGEP = dyn_cast<GetElementPtrInst>(instruction)) {
      handleGEPInst(pGEP, idx);
    } else if (BitCastInst *bitcast = dyn_cast<BitCastInst>(instruction)) {
      m_toBeRemoved.push_back(bitcast);
      Type *baseT =
          GetBaseType(bitcast->getType()->getPointerElementType(), nullptr);
      Type *sourceType = GetBaseType(
          bitcast->getOperand(0)->getType()->getPointerElementType(), nullptr);
      assert(baseT && sourceType);
      // either the point-to-element-type is the same or
      // the point-to-element-type is the byte
      if (baseT->getScalarSizeInBits() == sourceType->getScalarSizeInBits())
        HandleAllocaSources(bitcast, idx);
      else if (baseT->isPointerTy() && baseT->getPointerElementType()->isFunctionTy())
        HandleAllocaSources(bitcast, idx);
      else {
        assert(baseT->getScalarSizeInBits() == 8);
        IRBuilder<> IRB(bitcast);
        auto ElementSize =
            sourceType->getScalarSizeInBits() / baseT->getScalarSizeInBits();
        Value * Scale = nullptr;
        if (idx->getType()->isVectorTy()) {
          auto Width = idx->getType()->getVectorNumElements();
          Scale = ConstantVector::getSplat(Width, IRB.getInt32(ElementSize));
        }
        else
          Scale = IRB.getInt32(ElementSize);
        auto NewIdx = IRB.CreateMul(idx, Scale);
        HandleAllocaSources(bitcast, NewIdx);
      }
    } else if (StoreInst *pStore = llvm::dyn_cast<StoreInst>(instruction)) {
      handleStoreInst(pStore, idx);
    } else if (LoadInst *pLoad = llvm::dyn_cast<LoadInst>(instruction)) {
      handleLoadInst(pLoad, idx);
    } else if (PHINode *pPhi = llvm::dyn_cast<PHINode>(instruction)) {
      handlePHINode(pPhi, idx, v->getParent());
    } else if (IntrinsicInst *inst = dyn_cast<IntrinsicInst>(instruction)) {
      auto IID = GenXIntrinsic::getAnyIntrinsicID(inst);
      if (IID == llvm::Intrinsic::lifetime_start ||
          IID == llvm::Intrinsic::lifetime_end)
        inst->eraseFromParent();
      else if (IID == GenXIntrinsic::genx_gather_private)
        handlePrivateGather(inst, idx);
      else if (IID == GenXIntrinsic::genx_scatter_private)
        handlePrivateScatter(inst, idx);
      else if (inst->getIntrinsicID() == llvm::Intrinsic::masked_gather)
        handleLLVMGather(inst, idx);
      else if (inst->getIntrinsicID() == llvm::Intrinsic::masked_scatter)
        handleLLVMScatter(inst, idx);
    }
  }
}


void TransposeHelper::handleGEPInst(llvm::GetElementPtrInst *GEP,
                                    llvm::Value *idx) {
  m_toBeRemoved.push_back(GEP);
  Value *PtrOp = GEP->getPointerOperand();
  PointerType *PtrTy = dyn_cast<PointerType>(PtrOp->getType());
  assert(PtrTy && "Only accept scalar pointer!");
  int IdxWidth = 1;
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI) {
    Value * Idx = *OI;
    if (Idx->getType()->isVectorTy()) {
      auto Width = Idx->getType()->getVectorNumElements();
      if (Width > 1) {
        if (IdxWidth <= 1)
          IdxWidth = Width;
        else
          assert(IdxWidth == Width && "GEP has inconsistent vector-index width");
      }
    }
  }
  Type *Ty = PtrTy;
  gep_type_iterator GTI = gep_type_begin(GEP);
  IRBuilder<> IRB(GEP);
  Value * pScalarizedIdx = (IdxWidth == 1) ? IRB.getInt32(0) :
  ConstantVector::getSplat(IdxWidth, IRB.getInt32(0));
  for (auto OI = GEP->op_begin() + 1, E = GEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *Idx = *OI;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      unsigned Field = unsigned(cast<ConstantInt>(Idx)->getZExtValue());
      if (Field) {
        Constant *OffsetVal =
            IRB.getInt32(m_pDL->getStructLayout(StTy)->getElementOffset(Field) /
                         m_baseTypeAllocSize);
        if (IdxWidth > 1)
          OffsetVal = ConstantVector::getSplat(IdxWidth, OffsetVal);
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, OffsetVal);
      }
      Ty = StTy->getElementType(Field);
    } else {
      Ty = GTI.getIndexedType();
      if (const ConstantInt *CI = dyn_cast<ConstantInt>(Idx)) {
        if (!CI->isZero()) {
          Constant *OffsetVal =
              IRB.getInt32(m_pDL->getTypeAllocSize(Ty) * CI->getZExtValue() /
                           m_baseTypeAllocSize);
          if (IdxWidth > 1)
            OffsetVal = ConstantVector::getSplat(IdxWidth, OffsetVal);
          pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, OffsetVal);
        }
      }
      else if (!Idx->getType()->isVectorTy() && IdxWidth <= 1) {
        Value *NewIdx = IRB.CreateZExtOrTrunc(Idx, IRB.getInt32Ty());
        auto ElementSize = m_pDL->getTypeAllocSize(Ty) / m_baseTypeAllocSize;
        NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, NewIdx);
      } else {
        // the input idx is a vector or the one of the GEP index is vector
        Value * NewIdx = nullptr;
        auto ElementSize = m_pDL->getTypeAllocSize(Ty) / m_baseTypeAllocSize;
        if (Idx->getType()->isVectorTy()) {
          assert(Idx->getType()->getVectorNumElements() == IdxWidth);
          NewIdx = IRB.CreateZExtOrTrunc(Idx, pScalarizedIdx->getType());
          NewIdx = IRB.CreateMul(NewIdx,
            ConstantVector::getSplat(IdxWidth, IRB.getInt32(ElementSize)));
        }
        else {
          Value * NewIdx = IRB.CreateZExtOrTrunc(Idx, IRB.getInt32Ty());
          NewIdx = IRB.CreateMul(NewIdx, IRB.getInt32(ElementSize));
          // splat the new-idx into a vector
          NewIdx = IRB.CreateVectorSplat(IdxWidth, NewIdx);
        }
        pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, NewIdx);
      }
    }
  }
  if (!idx->getType()->isVectorTy() && IdxWidth <= 1) {
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, idx);
  }
  else if (idx->getType()->isVectorTy()) {
    assert(idx->getType()->getVectorNumElements() == IdxWidth);
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, idx);
  }
  else {
    auto SplatIdx = IRB.CreateVectorSplat(IdxWidth, idx);
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, SplatIdx);
  }
  HandleAllocaSources(GEP, pScalarizedIdx);
}

// Pass acummulated idx through new phi
void TransposeHelper::handlePHINode(PHINode *pPhi, Value *idx,
                                    BasicBlock *pIncomingBB) {
  PHINode *NewPhi = nullptr;
  // If phi is not yet visited
  if (!m_phiReplacement.count(pPhi)) {
    IRBuilder<> IRB(pPhi);
    NewPhi = IRB.CreatePHI(idx->getType(), pPhi->getNumIncomingValues(), "idx");
    m_phiReplacement.insert(std::make_pair(pPhi, NewPhi));
    m_toBeRemoved.push_back(pPhi);
  } else
    NewPhi = m_phiReplacement[pPhi];
  NewPhi->addIncoming(idx, pIncomingBB);
  HandleAllocaSources(pPhi, NewPhi);
}

void TransposeHelperPromote::handleLoadInst(LoadInst *pLoad,
                                            Value *pScalarizedIdx) {
  assert(pLoad->isSimple());
  IRBuilder<> IRB(pLoad);
  Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  auto LdTy = pLoad->getType()->getScalarType();
  auto VETy = pLoadVecAlloca->getType()->getScalarType();
  auto ReadIn = pLoadVecAlloca;
  bool IsFuncPointer = pLoad->getPointerOperandType()->isPointerTy() &&
    pLoad->getPointerOperandType()->getPointerElementType()->isPointerTy() &&
    pLoad->getPointerOperandType()->getPointerElementType()->getPointerElementType()->isFunctionTy();
  // do the type-casting if necessary
  if (VETy != LdTy && !IsFuncPointer) {
    auto VLen = pLoadVecAlloca->getType()->getVectorNumElements();
    assert(VETy->getScalarSizeInBits() >= LdTy->getScalarSizeInBits());
    assert((VETy->getScalarSizeInBits() % LdTy->getScalarSizeInBits()) == 0);
    VLen = VLen * (VETy->getScalarSizeInBits() / LdTy->getScalarSizeInBits());
    ReadIn = IRB.CreateBitCast(ReadIn, VectorType::get(LdTy, VLen));
  }
  if (IsFuncPointer) {
    Region R(VectorType::get(
                 pVecAlloca->getType()
                     ->getPointerElementType()
                     ->getVectorElementType(),
                 m_pDL->getTypeSizeInBits(LdTy) /
                     m_pDL->getTypeSizeInBits(pVecAlloca->getType()
                                                  ->getPointerElementType()
                                                  ->getVectorElementType())),
             m_pDL);
    if (!pScalarizedIdx->getType()->isIntegerTy(16)) {
      pScalarizedIdx = IRB.CreateZExtOrTrunc(pScalarizedIdx, Type::getInt16Ty(pLoad->getContext()));
    }
    R.Indirect = pScalarizedIdx;
    auto *Result = R.createRdRegion(pLoadVecAlloca, pLoad->getName(), pLoad,
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
    auto Len = pLoad->getType()->getVectorNumElements();
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

void TransposeHelperPromote::handleStoreInst(llvm::StoreInst *pStore,
                                             llvm::Value *pScalarizedIdx) {
  // Add Store instruction to remove list
  assert(pStore->isSimple());
  IRBuilder<> IRB(pStore);
  llvm::Value *pStoreVal = pStore->getValueOperand();
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  llvm::Value *WriteOut = pLoadVecAlloca;
  auto StTy = pStoreVal->getType()->getScalarType();
  auto VETy = pLoadVecAlloca->getType()->getScalarType();
  // do the type-casting if necessary

  bool IsFuncPointerStore =
      (isFuncPointerVec(pStoreVal) ||
       (pStoreVal->getType()->isPointerTy() &&
        pStoreVal->getType()->getPointerElementType()->isFunctionTy()));
  if (VETy != StTy && !IsFuncPointerStore) {
    auto VLen = pLoadVecAlloca->getType()->getVectorNumElements();
    assert(VETy->getScalarSizeInBits() >= StTy->getScalarSizeInBits());
    assert((VETy->getScalarSizeInBits()%StTy->getScalarSizeInBits()) == 0);
    VLen = VLen * (VETy->getScalarSizeInBits() / StTy->getScalarSizeInBits());
    WriteOut = IRB.CreateBitCast(WriteOut, VectorType::get(StTy, VLen));
  }
  if (IsFuncPointerStore) {
    auto *NewStoreVal = pStoreVal;
    assert(pVecAlloca->getType()->getPointerElementType()->getVectorElementType()->isIntegerTy(8));
    if (NewStoreVal->getType()->isPointerTy() &&
        NewStoreVal->getType()->getPointerElementType()->isFunctionTy()) {
      NewStoreVal = IRB.CreatePtrToInt(NewStoreVal, IntegerType::get(pStore->getContext(), 64));
      NewStoreVal = IRB.CreateBitCast(NewStoreVal, VectorType::get(VETy, 8));
    }
    Region R(NewStoreVal, m_pDL);
    if (!pScalarizedIdx->getType()->isIntegerTy(16)) {
      pScalarizedIdx = IRB.CreateZExtOrTrunc(pScalarizedIdx, Type::getInt16Ty(pStore->getContext()));
    }
    R.Indirect = pScalarizedIdx;
    WriteOut = R.createWrRegion(WriteOut, NewStoreVal, pStore->getName(), pStore,
                     pStore->getDebugLoc());
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
    auto Len = pStoreVal->getType()->getVectorNumElements();
    for (unsigned i = 0; i < Len; ++i) {
      Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
      auto Val = IRB.CreateExtractElement(pStoreVal, VectorIdx);
      auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
      WriteOut = IRB.CreateInsertElement(WriteOut, Val, Idx);
    }
  } else {
    WriteOut = IRB.CreateInsertElement(WriteOut, pStoreVal, pScalarizedIdx);
  }
  // cast the vector type back if necessary
  if (VETy != StTy)
    WriteOut = IRB.CreateBitCast(WriteOut, pLoadVecAlloca->getType());
  IRB.CreateStore(WriteOut, pVecAlloca);
  pStore->eraseFromParent();
}

void TransposeHelperPromote::handlePrivateGather(IntrinsicInst *pInst,
                                          Value *pScalarizedIdx) {
  IRBuilder<> IRB(pInst);
  assert(pInst->getType()->isVectorTy());
  Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  auto N = pInst->getType()->getVectorNumElements();
  auto ElemType = pInst->getType()->getVectorElementType();

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
  assert(GatherPtrTy);
  unsigned GatherPtrNumBytes =
      GatherPtrTy->getElementType()->getPrimitiveSizeInBits() / 8;
  if (CI != nullptr &&
      IsLinearVectorConstantInts(pInst->getArgOperand(2), v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    int BytesOffset = CI->getSExtValue() * GatherPtrNumBytes;
    R.Offset = v0 + BytesOffset;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  } else {
    auto OffsetType =
        VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pInst->getArgOperand(2), OffsetType, true);
    auto Cast = IRB.CreateIntCast(
        pScalarizedIdx, IntegerType::getInt16Ty(pInst->getContext()), true);
    auto Scale = IRB.CreateMul(IRB.getInt16(GatherPtrNumBytes), Cast);
    auto vec = VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), 1);
    auto GEPOffsets =
        IRB.CreateInsertElement(UndefValue::get(vec), Scale, IRB.getInt32(0));
    GEPOffsets = IRB.CreateShuffleVector(
        GEPOffsets, UndefValue::get(vec),
        ConstantAggregateZero::get(
            VectorType::get(IntegerType::getInt32Ty(pInst->getContext()), N)));
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
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  if (pStoreVal->getType()->isVectorTy() == false) {
    assert(false);
    return;
  }
  auto N = pStoreVal->getType()->getVectorNumElements();
  auto ElemType = pStoreVal->getType()->getVectorElementType();
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
  PointerType* ScatterPtrTy =
	  dyn_cast<PointerType>(pInst->getArgOperand(1)->getType());
  // pScalarizedIdx is an indice of element, so
  // count byte offset depending on the type of pointer in scatter
  assert(ScatterPtrTy);
  unsigned ScatterPtrNumBytes =
      ScatterPtrTy->getElementType()->getPrimitiveSizeInBits() / 8;
  if (CI != nullptr && IsLinearVectorConstantInts(pInst->getArgOperand(2), v0, diff)) {
    R.Indirect = nullptr;
    R.Width = N;
    int BytesOffset = CI->getSExtValue() * ScatterPtrNumBytes;
    R.Offset = v0 + BytesOffset;
    R.Stride = (diff * 8) / ElemType->getPrimitiveSizeInBits();
    R.VStride = 0;
  } else {
    auto OffsetType =
        VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pInst->getArgOperand(2), OffsetType, true);
    auto Cast = IRB.CreateIntCast(
        pScalarizedIdx, IntegerType::getInt16Ty(pInst->getContext()), true);
    auto Scale = IRB.CreateMul(IRB.getInt16(ScatterPtrNumBytes), Cast);
    auto vec = VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), 1);
    auto GEPOffsets =
        IRB.CreateInsertElement(UndefValue::get(vec), Scale, IRB.getInt32(0));
    GEPOffsets = IRB.CreateShuffleVector(
        GEPOffsets, UndefValue::get(vec),
        ConstantAggregateZero::get(
            VectorType::get(IntegerType::getInt32Ty(pInst->getContext()), N)));
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
  assert(pInst->getType()->isVectorTy());
  Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  auto N = pInst->getType()->getVectorNumElements();
  auto ElemType = pInst->getType()->getVectorElementType();

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
    auto OffsetType =
      VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pScalarizedIdx, OffsetType, false);
    auto ScaleVec =
      IRB.CreateInsertElement(UndefValue::get(OffsetType), IRB.getInt16(ElemNumBytes), IRB.getInt32(0));
    ScaleVec = IRB.CreateShuffleVector(
      ScaleVec, UndefValue::get(OffsetType),
      ConstantAggregateZero::get(
        VectorType::get(IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateMul(Offsets, ScaleVec);
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
  llvm::Value *pStoreVal = pInst->getArgOperand(3);
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca);
  if (pStoreVal->getType()->isVectorTy() == false) {
    assert(false);
    return;
  }
  auto N = pStoreVal->getType()->getVectorNumElements();
  auto ElemType = pStoreVal->getType()->getVectorElementType();
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
    auto OffsetType =
      VectorType::get(IntegerType::getInt16Ty(pInst->getContext()), N);
    auto Offsets = IRB.CreateIntCast(pScalarizedIdx, OffsetType, false);
    auto ScaleVec = IRB.CreateInsertElement(UndefValue::get(OffsetType),
      IRB.getInt16(ElemNumBytes),
      IRB.getInt32(0));
    ScaleVec = IRB.CreateShuffleVector(
      ScaleVec, UndefValue::get(OffsetType),
      ConstantAggregateZero::get(
        VectorType::get(IntegerType::getInt32Ty(pInst->getContext()), N)));
    Offsets = IRB.CreateMul(Offsets, ScaleVec);
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

} // namespace
