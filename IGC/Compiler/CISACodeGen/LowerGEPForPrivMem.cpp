/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "PointersSettings.h"
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/CISACodeGen/RegisterPressureEstimate.hpp"
#include "common/LLVMUtils.h"
#include "Compiler/CISACodeGen/LowerGEPForPrivMem.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "common/LLVMWarningsPush.hpp"
#include "llvmWrapper/IR/DerivedTypes.h"
#include "llvmWrapper/IR/IRBuilder.h"
#include "llvm/IR/GetElementPtrTypeIterator.h"
#include <llvm/IR/Function.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Transforms/Utils/Local.h>
#include <llvmWrapper/ADT/Optional.h>
#include "common/LLVMWarningsPop.hpp"
#include "Probe/Assertion.h"

#include <algorithm>

#define MAX_ALLOCA_PROMOTE_GRF_NUM 48
#define MAX_PRESSURE_GRF_NUM 90

using namespace llvm;
using namespace IGC;
using namespace IGC::IGCMD;

namespace IGC {
/// @brief  LowerGEPForPrivMem pass is used for lowering the allocas identified while visiting the alloca instructions
///         and then inserting insert/extract elements instead of load stores. This allows us
///         to store the data in registers instead of propagating it to scratch space.
class LowerGEPForPrivMem : public llvm::FunctionPass, public llvm::InstVisitor<LowerGEPForPrivMem> {
public:
  LowerGEPForPrivMem();

  ~LowerGEPForPrivMem() {}

  virtual StringRef getPassName() const override { return IGCOpts::LowerGEPForPrivMemPass; }

  virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override {
    AU.addRequired<RegisterPressureEstimate>();
    AU.addRequired<MetaDataUtilsWrapper>();
    AU.addRequired<CodeGenContextWrapper>();
    AU.addRequired<DominatorTreeWrapperPass>();
    AU.setPreservesCFG();
  }

  virtual bool runOnFunction(llvm::Function &F) override;

  void visitAllocaInst(llvm::AllocaInst &I);

  unsigned int extractConstAllocaSize(llvm::AllocaInst *pAlloca);

  static bool IsVariableSizeAlloca(llvm::AllocaInst &pAlloca);

private:
  llvm::AllocaInst *createVectorForAlloca(llvm::AllocaInst *pAlloca, llvm::Type *pBaseType);
  void handleAllocaInst(llvm::AllocaInst *pAlloca);

  StatusPrivArr2Reg CheckIfAllocaPromotable(llvm::AllocaInst *pAlloca);
  bool IsNativeType(Type *type);

  void MarkNotPromtedAllocas(llvm::AllocaInst &I, IGC::StatusPrivArr2Reg status);

public:
  static char ID;

  struct PromotedLiverange {
    unsigned int lowId;
    unsigned int highId;
    unsigned int varSize;
    RegisterPressureEstimate::LiveRange *LR;
  };

private:
  const llvm::DataLayout *m_pDL = nullptr;
  CodeGenContext *m_ctx = nullptr;
  DominatorTree *m_DT = nullptr;
  std::vector<llvm::AllocaInst *> m_allocasToPrivMem;
  RegisterPressureEstimate *m_pRegisterPressureEstimate = nullptr;
  llvm::Function *m_pFunc = nullptr;
  MetaDataUtils *pMdUtils = nullptr;

  /// Keep track of each BB affected by promoting MemtoReg and the current pressure at that block
  llvm::DenseMap<llvm::BasicBlock *, unsigned> m_pBBPressure;

  std::vector<PromotedLiverange> m_promotedLiveranges;
};

FunctionPass *createPromotePrivateArrayToReg() { return new LowerGEPForPrivMem(); }
} // namespace IGC

// Register pass to igc-opt
#define PASS_FLAG "igc-priv-mem-to-reg"
#define PASS_DESCRIPTION "Lower GEP of Private Memory to Register Pass"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(LowerGEPForPrivMem, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(RegisterPressureEstimate)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(LowerGEPForPrivMem, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char LowerGEPForPrivMem::ID = 0;

LowerGEPForPrivMem::LowerGEPForPrivMem() : FunctionPass(ID), m_pFunc(nullptr) {
  initializeLowerGEPForPrivMemPass(*PassRegistry::getPassRegistry());
}

llvm::AllocaInst *LowerGEPForPrivMem::createVectorForAlloca(llvm::AllocaInst *pAlloca, llvm::Type *pBaseType) {
  IGC_ASSERT(pAlloca != nullptr);
  IGCLLVM::IRBuilder<> IRB(pAlloca);
  AllocaInst *pAllocaValue = nullptr;
  if (IsVariableSizeAlloca(*pAlloca)) {
    pAllocaValue = IRB.CreateAlloca(pBaseType, pAlloca->getArraySize());

  } else {
    IGC_ASSERT(nullptr != m_pDL);
    const unsigned int denominator = int_cast<unsigned int>(m_pDL->getTypeAllocSize(pBaseType));
    IGC_ASSERT(0 < denominator);
    const unsigned int totalSize = extractConstAllocaSize(pAlloca) / denominator;
    pAllocaValue = IRB.CreateAlloca(IGCLLVM::FixedVectorType::get(pBaseType, totalSize));
  }

  return pAllocaValue;
}

bool LowerGEPForPrivMem::runOnFunction(llvm::Function &F) {
  m_pFunc = &F;
  CodeGenContextWrapper *pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
  IGC_ASSERT(nullptr != pCtxWrapper);
  m_ctx = pCtxWrapper->getCodeGenContext();
  m_DT = &getAnalysis<DominatorTreeWrapperPass>().getDomTree();

  if (isOptDisabledForFunction(m_ctx->getModuleMetaData(), getPassName(), &F))
    return false;

  pMdUtils = getAnalysis<MetaDataUtilsWrapper>().getMetaDataUtils();
  IGC_ASSERT(nullptr != pMdUtils);
  if (pMdUtils->findFunctionsInfoItem(&F) == pMdUtils->end_FunctionsInfo()) {
    return false;
  }
  IGC_ASSERT(nullptr != F.getParent());
  m_pDL = &F.getParent()->getDataLayout();
  m_pRegisterPressureEstimate = &getAnalysis<RegisterPressureEstimate>();
  IGC_ASSERT(nullptr != m_pRegisterPressureEstimate);
  // if no live range info
  if (!m_pRegisterPressureEstimate->isAvailable()) {
    return false;
  }
  m_pRegisterPressureEstimate->buildRPMapPerInstruction();

  m_allocasToPrivMem.clear();
  m_promotedLiveranges.clear();
  visit(F);

  std::vector<llvm::AllocaInst *> &allocaToHande = m_allocasToPrivMem;
  for (auto pAlloca : allocaToHande) {
    handleAllocaInst(pAlloca);
  }

  // Last remove alloca instructions
  for (auto pInst : allocaToHande) {
    if (pInst->use_empty()) {
      pInst->eraseFromParent();
    }
  }

  // IR changed only if we had alloca instruction to optimize
  return !allocaToHande.empty();
}

void TransposeHelper::EraseDeadCode() {
  for (auto pInst = m_toBeRemovedGEP.rbegin(); pInst != m_toBeRemovedGEP.rend(); ++pInst) {
    Instruction *I = *pInst;
    IGC_ASSERT_MESSAGE(I->use_empty(), "Instruction still has usage");
    (*pInst)->eraseFromParent();
  }
}

bool LowerGEPForPrivMem::IsVariableSizeAlloca(llvm::AllocaInst &pAlloca) {
  IGC_ASSERT(nullptr != pAlloca.getArraySize());
  if (isa<ConstantInt>(pAlloca.getArraySize()))
    return false;
  return true;
}

unsigned int LowerGEPForPrivMem::extractConstAllocaSize(llvm::AllocaInst *pAlloca) {
  IGC_ASSERT(nullptr != m_pDL);
  IGC_ASSERT(nullptr != pAlloca);
  IGC_ASSERT(nullptr != pAlloca->getArraySize());
  IGC_ASSERT(nullptr != pAlloca->getAllocatedType());
  unsigned int arraySize = int_cast<unsigned int>(cast<ConstantInt>(pAlloca->getArraySize())->getZExtValue());
  unsigned int totalArrayStructureSize =
      int_cast<unsigned int>(m_pDL->getTypeAllocSize(pAlloca->getAllocatedType()) * arraySize);

  return totalArrayStructureSize;
}

static void GetAllocaLiverange(Instruction *I, unsigned int &liverangeStart, unsigned int &liverangeEnd,
                               RegisterPressureEstimate *rpe,
                               SmallVector<LowerGEPForPrivMem::PromotedLiverange, 16> &GEPliveranges) {
  IGC_ASSERT(nullptr != I);

  for (Value::user_iterator use_it = I->user_begin(), use_e = I->user_end(); use_it != use_e; ++use_it) {
    if (isa<GetElementPtrInst>(*use_it) || isa<BitCastInst>(*use_it)) {
      // collect liveranges for GEP operations related to alloca
      Instruction *Inst = cast<Instruction>(*use_it);
      LowerGEPForPrivMem::PromotedLiverange GEPliverange;
      GEPliverange.LR = rpe->getLiveRangeOrNull(Inst);
      GEPliverange.lowId = GEPliverange.highId = rpe->getAssignedNumberForInst(Inst);
      GetAllocaLiverange(Inst, GEPliverange.lowId, GEPliverange.highId, rpe, GEPliveranges);
      GEPliverange.varSize = rpe->getRegisterWeightForInstruction(Inst);

      if (GEPliverange.LR)
        GEPliveranges.push_back(GEPliverange);

      liverangeStart = std::min(liverangeStart, GEPliverange.lowId);
      liverangeEnd = std::max(liverangeEnd, GEPliverange.highId);
    } else if (isa<LoadInst>(*use_it) || isa<StoreInst>(*use_it) || isa<llvm::IntrinsicInst>(*use_it)) {
      unsigned int idx = rpe->getAssignedNumberForInst(cast<Instruction>(*use_it));
      liverangeStart = std::min(liverangeStart, idx);
      liverangeEnd = std::max(liverangeEnd, idx);
    }
  }
}

bool LowerGEPForPrivMem::IsNativeType(Type *type) {
  if (type->isDoubleTy() && m_ctx->platform.hasNoFP64Inst()) {
    return false;
  }

  if (type->isIntegerTy(8) && (IGC_IS_FLAG_ENABLED(ForcePromoteI8) ||
                               (IGC_IS_FLAG_ENABLED(EnablePromoteI8) && !m_ctx->platform.supportByteALUOperation()))) {
    // Byte indirect: not supported for Vx1 and VxH on PVC.
    // As GRF from promoted privMem may use indirect accesses, disable it
    // to prevent Vx1 and VxH accesses.
    return false;
  }

  if (type->isStructTy())
    return false;

  return true;
}

StatusPrivArr2Reg LowerGEPForPrivMem::CheckIfAllocaPromotable(llvm::AllocaInst *pAlloca) {
  // vla is not promotable
  IGC_ASSERT(pAlloca != nullptr);
  if (IsVariableSizeAlloca(*pAlloca))
    return StatusPrivArr2Reg::IsDynamicAlloca;

  bool isUniformAlloca = pAlloca->getMetadata("uniform") != nullptr;
  bool useAssumeUniform = pAlloca->getMetadata("UseAssumeUniform") != nullptr;
  unsigned int allocaSize = extractConstAllocaSize(pAlloca);
  unsigned int allowedAllocaSizeInBytes = MAX_ALLOCA_PROMOTE_GRF_NUM * 4;
  unsigned int SIMDSize = numLanes(m_ctx->platform.getMinDispatchMode());

  // consider GRF width in alloca register promotion limit
  allowedAllocaSizeInBytes = allowedAllocaSizeInBytes * m_ctx->platform.getGRFSize() / 32;

  // scale alloc size based on the number of GRFs we have
  float grfRatio = m_ctx->getNumGRFPerThread() / 128.0f;
  allowedAllocaSizeInBytes = (uint32_t)(allowedAllocaSizeInBytes * grfRatio);

    if (m_ctx->type == ShaderType::OPENCL_SHADER) {
      FunctionInfoMetaDataHandle funcInfoMD = pMdUtils->getFunctionsInfoItem(m_pFunc);
      SubGroupSizeMetaDataHandle subGroupSize = funcInfoMD->getSubGroupSize();
      if (subGroupSize->hasValue()) {
        SIMDSize = std::max((uint32_t)subGroupSize->getSIMDSize(), SIMDSize);
      }

      allowedAllocaSizeInBytes = (allowedAllocaSizeInBytes * 8) / SIMDSize;
    }
  SOALayoutChecker checker(*pAlloca, m_ctx->type == ShaderType::OPENCL_SHADER);
  SOALayoutInfo SOAInfo = checker.getOrGatherInfo();
  if (!SOAInfo.canUseSOALayout) {
    return StatusPrivArr2Reg::CannotUseSOALayout;
  }
  if (!IsNativeType(SOAInfo.baseType)) {
    return StatusPrivArr2Reg::IsNotNativeType;
  }
  if (isUniformAlloca) {
    // Heuristic: for uniform alloca we divide the size by SIMDSize to adjust the pressure
    // as they will be allocated as uniform array
    allocaSize = iSTD::Round(allocaSize, SIMDSize) / SIMDSize;
  }

  if (useAssumeUniform || allocaSize <= IGC_GET_FLAG_VALUE(ByPassAllocaSizeHeuristic)) {
    return StatusPrivArr2Reg::OK;
  }

  // if alloca size exceeds alloc size threshold, return false
  if (allocaSize > allowedAllocaSizeInBytes) {
    return StatusPrivArr2Reg::OutOfAllocSizeLimit;
  }

  // Multiple indirect byte access could be harmful for performance
  if (SOAInfo.baseType->getScalarType()->isIntegerTy(8) && !isUniformAlloca &&
      m_ctx->platform.isCoreChildOf(IGFX_XE_HPG_CORE)) {
    // Limit promotable alloca size with byte indirect access by 4 GRF vector size
    if (allocaSize * SIMDSize / m_ctx->platform.getGRFSize() > 4)
      return StatusPrivArr2Reg::IsNotNativeType;
  }

  // get all the basic blocks that contain the uses of the alloca
  // then estimate how much changing this alloca to register adds to the pressure at that block.
  unsigned int lowestAssignedNumber = 0xFFFFFFFF;
  unsigned int highestAssignedNumber = 0;
  SmallVector<PromotedLiverange, 16> GEPliveranges;

  GetAllocaLiverange(pAlloca, lowestAssignedNumber, highestAssignedNumber, m_pRegisterPressureEstimate, GEPliveranges);

  uint32_t maxGRFPressure = (uint32_t)(grfRatio * MAX_PRESSURE_GRF_NUM * 4);

  unsigned int pressure = 0;
  for (unsigned int i = lowestAssignedNumber; i <= highestAssignedNumber; i++) {
    // subtract impact from GEP operations related to alloca from the register pressure
    // since after promotion alloca to register these GEPs will be eliminated
    unsigned int GEPImpact = 0;
    for (const auto &GEPinst : GEPliveranges) {
      if (GEPinst.LR->contains(i))
        GEPImpact += GEPinst.varSize;
    }

    unsigned RPinst = m_pRegisterPressureEstimate->getRegisterPressureForInstructionFromRPMap(i);
    pressure = std::max(pressure, RPinst - GEPImpact);
  }

  for (const auto &it : m_promotedLiveranges) {
    // check interval intersection
    if ((it.lowId < lowestAssignedNumber && it.highId > lowestAssignedNumber) ||
        (it.lowId > lowestAssignedNumber && it.lowId < highestAssignedNumber)) {
      pressure += it.varSize;
    }
  }

  if (allocaSize + pressure > maxGRFPressure) {
    return StatusPrivArr2Reg::OutOfMaxGRFPressure;
  }
  PromotedLiverange liverange;
  liverange.lowId = lowestAssignedNumber;
  liverange.highId = highestAssignedNumber;
  liverange.varSize = allocaSize;
  liverange.LR = nullptr;
  m_promotedLiveranges.push_back(liverange);
  return StatusPrivArr2Reg::OK;
}

SOALayoutChecker::SOALayoutChecker(AllocaInst &allocaToCheck, bool isOCL) : allocaRef(allocaToCheck) {
  auto F = allocaToCheck.getParent()->getParent();
  pDL = &F->getParent()->getDataLayout();
  newAlgoControl = IGC_GET_FLAG_VALUE(EnablePrivMemNewSOATranspose);
  if (IGC_IS_FLAG_ENABLED(NewSOATransposeForOpenCL) && !isOCL) {
    newAlgoControl = 0;
  }
}

SOALayoutInfo SOALayoutChecker::getOrGatherInfo() {
  if (pInfo)
    return *pInfo;
  pInfo = std::make_unique<SOALayoutInfo>(false, nullptr, false, 4);

  // Do not allow SOA layout for vla which will be stored on the stack.
  // We don't support SOA layout for privates on stack at all so this is just to make
  // the implementation simpler.
  if (LowerGEPForPrivMem::IsVariableSizeAlloca(allocaRef))
    return *pInfo;

  // Don't even look at non-array allocas.
  // (extractAllocaDim can not handle them anyway, causing a crash)
  llvm::Type *pType = allocaRef.getAllocatedType();
  if (pType->isStructTy() && pType->getStructNumElements() == 1) {
    pType = pType->getStructElementType(0);
  }
  if ((!pType->isArrayTy() && !pType->isVectorTy()) || allocaRef.isArrayAllocation())
    return *pInfo;

  // Enable transpose for array of struct
  pInfo->baseType = GetBaseType(pType, newAlgoControl > 0 ? true : false);
  if (!pInfo->baseType)
    return *pInfo;

  if (useNewAlgo(pInfo->baseType)) {
    SOAPartitionBytes = selectPartitionSize(pInfo->baseType);

    StructType *STy = dyn_cast<StructType>(pInfo->baseType);
    if (STy != nullptr) {
      if (!isPowerOf2_32(SOAPartitionBytes) || !checkStruct(STy)) {
        if (newAlgoControl < 3)
          return *pInfo;

        // newAlgoControl = 3
        // check if partition size can be the entire struct
        uint32_t sz = (uint32_t)pDL->getTypeStoreSize(STy);
        if (sz > 16 || !isPowerOf2_32(sz)) {
          return *pInfo;
        }
        SOAPartitionBytes = std::max(4u, sz);
      }
    }

    // Skip for non-power-of-2 partition size
    if (isPowerOf2_32(SOAPartitionBytes)) {
      pInfo->canUseSOALayout = checkUsers(allocaRef);
      pInfo->SOAPartitionBytes = SOAPartitionBytes;
    }
    return *pInfo;
  }
  // only handle case with a simple base type
  if (!(pInfo->baseType->getScalarType()->isFloatingPointTy() || pInfo->baseType->getScalarType()->isIntegerTy()))
    return *pInfo;

  // Now that we've confirmed our alloca to be a valid candidate, assume that
  // all memory instructions are vector unless proven otherwise.
  pInfo->allUsesAreVector = true;
  // Start the traversal: each specified visitor function will delegate its
  // checked instruction to the same method if need be.
  pInfo->canUseSOALayout = checkUsers(allocaRef);
  if (!isVectorSOA) {
    pInfo->baseType = pInfo->baseType->getScalarType();
  }
  return *pInfo;
}

uint32_t SOALayoutChecker::selectPartitionSize(Type *Ty) {
  uint32_t size = 4;
  if (StructType *StTy = dyn_cast<StructType>(Ty)) {
    int nElts = (int)StTy->getNumElements();
    for (int ix = 0; ix < nElts; ++ix) {
      Type *eTy = StTy->getElementType(ix);
      uint32_t sz = selectPartitionSize(eTy);
      size = std::max(sz, size);
    }
    return size;
  }
  if (Ty->isArrayTy()) {
    // Don't split vector
    uint32_t sz = selectPartitionSize(Ty->getArrayElementType());
    size = std::max(sz, size);
    return size;
  }

  uint32_t sz = (uint32_t)pDL->getTypeStoreSize(Ty);
  size = std::max(sz, size);
  return size;
}

bool SOALayoutChecker::checkStruct(StructType *StTy) {
  if (!StTy->isSized())
    return false;

  uint32_t StTyBytes = (uint32_t)pDL->getTypeStoreSize(StTy);

  // Larger struct shall be multiple of partition size
  if (StTyBytes > SOAPartitionBytes && (StTyBytes % SOAPartitionBytes) != 0)
    return false;
  // Partition shall be multiple of smaller struct size
  if (StTyBytes < SOAPartitionBytes && (SOAPartitionBytes % StTyBytes) != 0)
    return false;

  const StructLayout *SL = pDL->getStructLayout(StTy);
  int32_t nElts = (int)StTy->getNumElements();
  for (int ix = 0; ix < nElts; ++ix) {
    Type *ty = StTy->getElementType(ix);
    uint32_t eTyBytes = (uint32_t)pDL->getTypeStoreSize(ty);
    IGC_ASSERT(SOAPartitionBytes >= eTyBytes || ty->isAggregateType());
    if (!isPowerOf2_32(eTyBytes))
      return false;

    if (newAlgoControl == 1) {
      // only handle struct with members being same-sized scalars
      if (SOAPartitionBytes != eTyBytes || !ty->isSingleValueType() || ty->isVectorTy()) {
        return false;
      }
    }
    // newAlgoControl=2 is handled in other places
    else if (newAlgoControl > 2) {
      // May handle nested struct/array, etc.
      if (!ty->isSingleValueType()) {
        return false;
      }
    }
    uint32_t byteOffset = (uint32_t)SL->getElementOffset(ix);
    uint32_t chunkOff = (byteOffset % SOAPartitionBytes);
    // check alignment
    if (MinAlign(eTyBytes, chunkOff) < eTyBytes) {
      return false;
    }
  }
  return true;
}

// TODO: Consider a worklist-based implementation instead.
bool SOALayoutChecker::checkUsers(Instruction &I) {
  if (IGC_IS_FLAG_ENABLED(DisableSOAPromotion)) {
    return false;
  }

  parentLevelInst = &I;
  for (Value::user_iterator userIt = I.user_begin(), userE = I.user_end(); userIt != userE; ++userIt) {
    auto &userInst = *cast<Instruction>(*userIt);
    if (!visit(userInst))
      return false;
  }
  return true;
}

bool SOALayoutChecker::visitBitCastInst(BitCastInst &BI) {
  if (BI.use_empty() || IsBitCastForLifetimeMark(&BI)) {
    return true;
  }

  Type *baseT = GetBaseType(IGCLLVM::getNonOpaquePtrEltTy(BI.getType()), true);
  Type *sourceType = GetBaseType(IGCLLVM::getNonOpaquePtrEltTy(BI.getOperand(0)->getType()), true);
  if (baseT->isStructTy() || sourceType->isStructTy()) {
    StructType *bSTy = dyn_cast<StructType>(baseT);
    StructType *sSTy = dyn_cast<StructType>(sourceType);
    IGC_ASSERT(bSTy || sSTy);
    if (bSTy && sSTy && (bSTy == sSTy || bSTy->isLayoutIdentical(sSTy))) {
      return checkUsers(BI);
    } else {
      return false;
    }
  }

  if (baseT->getScalarSizeInBits() != 0 && baseT->getScalarSizeInBits() == sourceType->getScalarSizeInBits()) {
    const bool sameSize = ((uint32_t)baseT->getPrimitiveSizeInBits() == (uint32_t)sourceType->getPrimitiveSizeInBits());
    isVectorSOA &= sameSize;
    if (newAlgoControl > 1 && baseT->isVectorTy() && !sameSize) {
      return false;
    }
    return checkUsers(BI);
  }

  // Not a candidate.
  return false;
}

bool SOALayoutChecker::visitGetElementPtrInst(GetElementPtrInst &GEP) { return checkUsers(GEP); }

bool SOALayoutChecker::visitIntrinsicInst(IntrinsicInst &II) {
  llvm::Intrinsic::ID IID = II.getIntrinsicID();
  return IID == llvm::Intrinsic::lifetime_start || IID == llvm::Intrinsic::lifetime_end;
}

// Detection of mismatch between type sizes of
// alloca -> load / store
// or
// alloca -> gep -> load / store
bool IGC::SOALayoutChecker::MismatchDetected(Instruction &I) {

  if (!isa<LoadInst>(I) && !isa<StoreInst>(I))
    return false;

  // Only detect mismatch if are have opaque pointers (LLVM>=16)
  if (!IGC::AreOpaquePointersEnabled())
    return false;

  if (!pInfo->baseType)
    return false;

  Type *allocaTy = allocaRef.getAllocatedType();
  bool allocaIsVecOrArr = allocaTy->isVectorTy() || allocaTy->isArrayTy();

  if (!allocaIsVecOrArr)
    return false;

  bool useOldAlgorithm = !useNewAlgo(pInfo->baseType);

  if (useOldAlgorithm) {
    auto DL = I.getParent()->getParent()->getParent()->getDataLayout();

    Type *pUserTy = I.getType();

    if (auto *storeInst = dyn_cast<StoreInst>(&I))
      pUserTy = storeInst->getValueOperand()->getType();

    if (auto *pgep = dyn_cast<GetElementPtrInst>(parentLevelInst)) {
      allocaTy = pgep->getResultElementType();
    } else {
      if (auto *arrTy = dyn_cast<ArrayType>(allocaTy)) {
        allocaTy = arrTy->getElementType();
      } else if (auto *vec = dyn_cast<IGCLLVM::FixedVectorType>(allocaTy)) {
        allocaTy = vec->getElementType();
      }

      if (auto *arrTy = dyn_cast<ArrayType>(pUserTy)) {
        pUserTy = arrTy->getElementType();
      } else if (auto *vec = dyn_cast<IGCLLVM::FixedVectorType>(pUserTy)) {
        pUserTy = vec->getElementType();
      }
    }

    auto allocaSize = DL.getTypeAllocSize(allocaTy);
    auto vecTySize = DL.getTypeAllocSize(pUserTy);

    if (vecTySize != allocaSize) {
      pInfo->canUseSOALayout = false;
      return true;
    }
  }
  return false;
}

bool SOALayoutChecker::visitLoadInst(LoadInst &LI) {
  bool isVectorLoad = LI.getType()->isVectorTy();
  isVectorSOA &= isVectorLoad;
  pInfo->allUsesAreVector &= isVectorLoad;

  if (MismatchDetected(LI))
    return false;

  return LI.isSimple();
}

bool SOALayoutChecker::visitStoreInst(StoreInst &SI) {
  if (!SI.isSimple())
    return false;
  llvm::Value *pValueOp = SI.getValueOperand();
  bool isVectorStore = pValueOp->getType()->isVectorTy();
  isVectorSOA &= isVectorStore;
  pInfo->allUsesAreVector &= isVectorStore;
  if (pValueOp == parentLevelInst) {
    // GEP instruction is the stored value of the StoreInst (unsupported case)
    return false;
  }

  if (MismatchDetected(SI))
    return false;

  return true;
}

void LowerGEPForPrivMem::MarkNotPromtedAllocas(llvm::AllocaInst &I, IGC::StatusPrivArr2Reg status) {
  const char *reason = nullptr;
  // The reason why the user private variable
  // wasn't promoted to grfs
  switch (status) {
  case StatusPrivArr2Reg::CannotUseSOALayout:
    reason = "CannotUseSOALayout";
    break;
  case StatusPrivArr2Reg::IsDynamicAlloca:
    reason = "IsDynamicAlloca";
    break;
  case StatusPrivArr2Reg::IsNotNativeType:
    reason = "IsNotNativeType";
    break;
  case StatusPrivArr2Reg::OutOfAllocSizeLimit:
    reason = "OutOfAllocSizeLimit";
    break;
  case StatusPrivArr2Reg::OutOfMaxGRFPressure:
    reason = "OutOfMaxGRFPressure";
    break;
  default:
    reason = "NotDefine";
    break;
  }
  MDNode *node = MDNode::get(I.getContext(), MDString::get(I.getContext(), reason));

  UserAddrSpaceMD &userASMD = m_ctx->getUserAddrSpaceMD();
  std::function<void(Instruction *, MDNode *)> markAS_PRIV;
  markAS_PRIV = [&markAS_PRIV, &userASMD](Instruction *instr, MDNode *node) {
    // Avoid instruction which has already md set
    if (!userASMD.Has(instr, LSC_DOC_ADDR_SPACE::PRIVATE)) {
      // Adding this mark because, during compilation the orginal
      // addrspace is changed (for ex. from PRIVATE to GLOBAL) and
      // is not visible on end stages of compilation. This will help
      // to identify - which load/store is related for the private
      // variables of user.

      bool isLoadStore = instr->getOpcode() == Instruction::Store || instr->getOpcode() == Instruction::Load;

      if (isLoadStore) {
        // Add mark for any load/store which will read/write the data from
        // user private variable. This information will be passed
        // to the assembly level.
        userASMD.Set(instr, LSC_DOC_ADDR_SPACE::PRIVATE, node);
      } else {
        // Special case to avoid stack overflow
        userASMD.Set(instr, LSC_DOC_ADDR_SPACE::PRIVATE);

        bool allowedInst = instr->getOpcode() == Instruction::Alloca || instr->getOpcode() == Instruction::PHI ||
                           instr->getOpcode() == Instruction::GetElementPtr ||
                           instr->getOpcode() == Instruction::PtrToInt || instr->getOpcode() == Instruction::IntToPtr ||
                           instr->isBinaryOp();

        if (allowedInst) {
          for (auto user_i = instr->user_begin(); user_i != instr->user_end(); ++user_i) {
            markAS_PRIV(llvm::dyn_cast<Instruction>(*user_i), node);
          }
        }
      }
    }
  };

  markAS_PRIV(&I, node);
}

void LowerGEPForPrivMem::visitAllocaInst(AllocaInst &I) {
  // Alloca should always be private memory
  IGC_ASSERT(nullptr != I.getType());
  IGC_ASSERT(I.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE);

  StatusPrivArr2Reg status = CheckIfAllocaPromotable(&I);
  if (I.getType()->getAddressSpace() == ADDRESS_SPACE_PRIVATE) {
    m_ctx->metrics.CollectMem2Reg(&I, status);
  }
  if (status != StatusPrivArr2Reg::OK) {
    MarkNotPromtedAllocas(I, status);
    // alloca size extends remain per-lane-reg space
    return;
  }
  m_allocasToPrivMem.push_back(&I);
}

void TransposeHelper::HandleAllocaSources(Instruction *v, Value *idx) {
  SmallVector<Value *, 10> instructions;
  for (Value::user_iterator it = v->user_begin(), e = v->user_end(); it != e; ++it) {
    Value *inst = cast<Value>(*it);
    instructions.push_back(inst);
  }

  for (auto instruction : instructions) {
    if (GetElementPtrInst *pGEP = dyn_cast<GetElementPtrInst>(instruction)) {
      handleGEPInst(pGEP, idx);
    } else if (BitCastInst *bitcast = dyn_cast<BitCastInst>(instruction)) {
      m_toBeRemovedGEP.push_back(bitcast);
      HandleAllocaSources(bitcast, idx);
    } else if (StoreInst *pStore = llvm::dyn_cast<StoreInst>(instruction)) {
      handleStoreInst(pStore, idx);
    } else if (LoadInst *pLoad = llvm::dyn_cast<LoadInst>(instruction)) {
      handleLoadInst(pLoad, idx);
    } else if (IntrinsicInst *inst = dyn_cast<IntrinsicInst>(instruction)) {
      handleLifetimeMark(inst);
    }
  }
}

class TransposeHelperPromote : public TransposeHelper {
public:
  void handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx);
  void handleStoreInst(StoreInst *pStore, Value *pScalarizedIdx);
  void handleLifetimeMark(IntrinsicInst *inst);
  AllocaInst *pVecAlloca;
  // location of lifetime starts
  llvm::SmallPtrSet<Instruction *, 4> pStartPoints;
  TransposeHelperPromote(AllocaInst *pAI, const DataLayout &DL) : TransposeHelper(DL, false) { pVecAlloca = pAI; }
};

void LowerGEPForPrivMem::handleAllocaInst(llvm::AllocaInst *pAlloca) {
  // Extract the Alloca size and the base Type
  Type *pType = pAlloca->getAllocatedType();
  Type *pBaseType = GetBaseType(pType)->getScalarType();
  IGC_ASSERT(pBaseType);
  llvm::AllocaInst *pVecAlloca = createVectorForAlloca(pAlloca, pBaseType);
  if (!pVecAlloca) {
    return;
  }

  IRBuilder<> IRB(pVecAlloca);
  Value *idx = IRB.getInt32(0);
  TransposeHelperPromote helper(pVecAlloca, *m_pDL);
  helper.HandleAllocaSources(pAlloca, idx);
  IGC_ASSERT(nullptr != pAlloca);
  // for uniform alloca, we need to insert an initial definition
  // to keep the promoted vector as uniform in the next round of WIAnalysis
  bool isUniformAlloca = pAlloca->getMetadata("uniform") != nullptr;
  if (isUniformAlloca && pAlloca->getAllocatedType()->isArrayTy()) {
    if (helper.pStartPoints.empty())
      helper.pStartPoints.insert(pAlloca);
    for (auto InsertionPoint : helper.pStartPoints) {
      IRBuilder<> IRB1(InsertionPoint);
      auto pVecF = GenISAIntrinsic::getDeclaration(m_pFunc->getParent(), GenISAIntrinsic::GenISA_vectorUniform,
                                                   pVecAlloca->getAllocatedType());
      auto pVecInit = IRB1.CreateCall(pVecF);
      // create a store of pVecInit into pVecAlloca
      IRB1.CreateStore(pVecInit, pVecAlloca);
    }
  }
  helper.EraseDeadCode();
  if (pAlloca->use_empty()) {
    IGC_ASSERT(m_DT);
    replaceAllDbgUsesWith(*pAlloca, *pVecAlloca, *pVecAlloca, *m_DT);
  }
}

std::pair<unsigned int, Type *> TransposeHelper::getArrSizeAndEltType(Type *T) {
  unsigned int arr_sz = 1;
  Type *retTy{};
  if (T->isStructTy()) {
    arr_sz = 1;
    retTy = T->getStructElementType(0);
  } else if (T->isArrayTy()) {
    arr_sz = int_cast<unsigned int>(T->getArrayNumElements());
    retTy = T->getArrayElementType();
  } else if (T->isVectorTy()) {
    // Based on whether we want the index in number of element or number of vector.
    // Vectors can only contain primitives or pointers, so there's no need for additional
    // logic to account for vectors contained in vectors.
    if (m_vectorIndex) {
      arr_sz = 1;
    } else {
      auto *vTy = cast<IGCLLVM::FixedVectorType>(T);
      unsigned int vector_size_in_bytes = int_cast<unsigned int>(m_DL.getTypeAllocSize(T));
      unsigned int elt_size_in_bytes = int_cast<unsigned int>(m_DL.getTypeAllocSize(vTy->getElementType()));
      arr_sz = vector_size_in_bytes / elt_size_in_bytes;
    }
    retTy = cast<VectorType>(T)->getElementType();
  } else {
    arr_sz = 1;
    retTy = T; // To preserve past behaviour
  }
  return std::make_pair(arr_sz, retTy);
}

Type *TransposeHelper::getFirstNonScalarSourceElementType(const GetElementPtrInst &GEP) {
  Type *currTy = GEP.getSourceElementType();
  if (getArrSizeAndEltType(currTy).first > 1)
    return currTy;

  const Value *base = GEP.getPointerOperand()->stripPointerCasts();

  if (const auto *AI = dyn_cast<AllocaInst>(base))
    return AI->getAllocatedType();
  if (const auto *GV = dyn_cast<GlobalVariable>(base))
    return GV->getValueType();
  if (const auto *LI = dyn_cast<LoadInst>(base))
    return LI->getType();
  if (const auto *SI = dyn_cast<StoreInst>(base))
    return SI->getValueOperand()->getType();

  return currTy;
}

void TransposeHelper::handleGEPInst(llvm::GetElementPtrInst *pGEP, llvm::Value *idx) {
  // TODO: Add support for GEP attributes: nsw, nuw, inbounds. Currently, neigher the old nor the new algorithm handles
  // them.
  if (useNewAlgo()) {
    handleGEPInstNew(pGEP, idx);
    return;
  }

  IGC_ASSERT(nullptr != pGEP);
  IGC_ASSERT(static_cast<ADDRESS_SPACE>(pGEP->getPointerAddressSpace()) == ADDRESS_SPACE_PRIVATE);
  // Add GEP instruction to remove list
  m_toBeRemovedGEP.push_back(pGEP);
  if (pGEP->use_empty()) {
    // GEP has no users, do nothing.
    return;
  }

  IRBuilder<> IRB(pGEP);
  Value *pScalarizedIdx = IRB.getInt32(0);

  // If the GEP is on i8, its index is a byte offset and must be converted to an element index of the underlying base
  // type.
  if (pGEP->getSourceElementType()->isIntegerTy(8)) {
    // Get the non-scalar/aggregate GEP source element type.
    Type *baseAggregateTy = getFirstNonScalarSourceElementType(*pGEP);
    // Find the scalar element type at the bottom of the aggregate.
    Type *elementTy = baseAggregateTy;
    while (elementTy->isStructTy() || elementTy->isArrayTy() || elementTy->isVectorTy()) {
      elementTy = getArrSizeAndEltType(elementTy).second;
    }
    elementTy = elementTy->getScalarType();
    uint32_t elementBytes = (uint32_t)m_DL.getTypeAllocSize(elementTy);

    // The 1st operand is the byte offset, convert bytes to element count.
    Value *byteIndex = IRB.CreateZExtOrTrunc(pGEP->getOperand(1), IRB.getInt32Ty());
    if (elementBytes > 1)
      byteIndex = IRB.CreateUDiv(byteIndex, IRB.getInt32(elementBytes));

    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, byteIndex);
    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, idx);
    HandleAllocaSources(pGEP, pScalarizedIdx);
    return;
  }

  // Given %p = getelementptr [4 x [3 x <2 x float>]]* %v, i64 0, i64 %1, i64 %2
  // compute the scalarized index with an auxiliary array [4, 3, 2]:
  //
  // Formula: index = (%1 x 3 + %2) x 2
  //
  Type *T = pGEP->getSourceElementType();
  for (unsigned i = 0, e = pGEP->getNumIndices(); i < e; ++i) {
    // If T is VectorType we should be at the last loop iteration. This will break things only if m_vectorIndex == true.
    IGC_ASSERT_MESSAGE(!m_vectorIndex || (!T->isVectorTy() || (i == (e - 1))),
                       "GEPs shouldn't index into vector elements.");
    // https://llvm.org/docs/GetElementPtr.html#can-gep-index-into-vector-elements

    auto GepOpnd = IRB.CreateZExtOrTrunc(pGEP->getOperand(i + 1), IRB.getInt32Ty());

    auto [arr_sz, eltTy] = getArrSizeAndEltType(T);

    pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, GepOpnd);
    pScalarizedIdx = IRB.CreateMul(pScalarizedIdx, IRB.getInt32(arr_sz));

    T = eltTy;
  }
  while (T->isStructTy() || T->isArrayTy() || T->isVectorTy()) {
    auto [arr_sz, eltTy] = getArrSizeAndEltType(T);

    pScalarizedIdx = IRB.CreateMul(pScalarizedIdx, IRB.getInt32(arr_sz));

    T = eltTy;
  }
  pScalarizedIdx = IRB.CreateAdd(pScalarizedIdx, idx);
  HandleAllocaSources(pGEP, pScalarizedIdx);
}

void TransposeHelper::handleGEPInstNew(llvm::GetElementPtrInst *pGEP, llvm::Value *idx) {
  IGC_ASSERT(nullptr != pGEP);
  IGC_ASSERT(static_cast<ADDRESS_SPACE>(pGEP->getPointerAddressSpace()) == ADDRESS_SPACE_PRIVATE);
  // Add GEP instruction to remove list
  m_toBeRemovedGEP.push_back(pGEP);
  if (pGEP->use_empty()) {
    // GEP has no users, do nothing.
    return;
  }

  auto isIntZero = [](Value *V) {
    ConstantInt *CI = dyn_cast<ConstantInt>(V);
    return (CI && CI->isZero());
  };

  IRBuilder<> IRB(pGEP);
  // linearOffset : byte offset of int32 (could be negative, no overflow).
  Value *linearOffset = IRB.getInt32(0);
  gep_type_iterator GTI = gep_type_begin(pGEP);
  for (auto OI = pGEP->op_begin() + 1, E = pGEP->op_end(); OI != E; ++OI, ++GTI) {
    Value *ix = *OI;
    Value *OffsetVal;
    if (StructType *StTy = GTI.getStructTypeOrNull()) {
      unsigned Field = int_cast<unsigned>(cast<ConstantInt>(ix)->getZExtValue());
      uint64_t Offset = m_DL.getStructLayout(StTy)->getElementOffset(Field);
      OffsetVal = IRB.getInt32((uint32_t)Offset);
    } else {
      Value *NewIx = IRB.CreateSExtOrTrunc(ix, linearOffset->getType());
      // OffsetVal = NewIx * tyBytes
      if (isIntZero(NewIx)) {
        OffsetVal = NewIx;
      } else {
        Type *Ty = GTI.getIndexedType();
        uint64_t tyBytes = m_DL.getTypeAllocSize(Ty);
        OffsetVal = IRB.CreateNSWMul(NewIx, IRB.getInt32((uint32_t)tyBytes));
      }
    }
    // linearOffset += OffsetVal
    if (!isIntZero(OffsetVal)) {
      if (isIntZero(linearOffset))
        linearOffset = OffsetVal;
      else
        linearOffset = IRB.CreateNSWAdd(linearOffset, OffsetVal);
    }
  }
  // linearOffset += idx
  if (!isIntZero(idx))
    linearOffset = IRB.CreateAdd(linearOffset, idx);
  HandleAllocaSources(pGEP, linearOffset);
}

// Load N elements from a vector alloca, Idx, ... Idx + N - 1. Return a scalar
// or a vector value depending on N.
static Value *loadEltsFromVecAlloca(unsigned N, AllocaInst *pVecAlloca, Value *pScalarizedIdx,
                                    IGCLLVM::IRBuilder<> &IRB, Type *scalarType) {
  Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca->getAllocatedType(), pVecAlloca);
  if (N == 1) {
    return IRB.CreateBitCast(IRB.CreateExtractElement(pLoadVecAlloca, pScalarizedIdx), scalarType);
  }

  // A vector load
  // %v = load <2 x float>* %ptr
  // becomes
  // %w = load <32 x float>* %ptr1
  // %v0 = extractelement <32 x float> %w, i32 %idx
  // %v1 = extractelement <32 x float> %w, i32 %idx+1
  // replace all uses of %v with <%v0, %v1>
  IGC_ASSERT_MESSAGE((N > 1), "out of sync");
  Type *Ty = IGCLLVM::FixedVectorType::get(scalarType, N);
  Value *Result = UndefValue::get(Ty);

  for (unsigned i = 0; i < N; ++i) {
    Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
    auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
    auto Val = IRB.CreateExtractElement(pLoadVecAlloca, Idx);
    Val = IRB.CreateBitCast(Val, scalarType);
    Result = IRB.CreateInsertElement(Result, Val, VectorIdx);
  }
  return Result;
}

void TransposeHelperPromote::handleLoadInst(LoadInst *pLoad, Value *pScalarizedIdx) {
  IGC_ASSERT(nullptr != pLoad);
  IGC_ASSERT(pLoad->isSimple());
  IGCLLVM::IRBuilder<> IRB(pLoad);
  IGC_ASSERT(nullptr != pLoad->getType());
  unsigned N =
      pLoad->getType()->isVectorTy() ? (unsigned)cast<IGCLLVM::FixedVectorType>(pLoad->getType())->getNumElements() : 1;
  Value *Val = loadEltsFromVecAlloca(N, pVecAlloca, pScalarizedIdx, IRB, pLoad->getType()->getScalarType());
  pLoad->replaceAllUsesWith(Val);
  pLoad->eraseFromParent();
}

void TransposeHelperPromote::handleStoreInst(llvm::StoreInst *pStore, llvm::Value *pScalarizedIdx) {
  // Add Store instruction to remove list
  IGC_ASSERT(nullptr != pStore);
  IGC_ASSERT(pStore->isSimple());

  IGCLLVM::IRBuilder<> IRB(pStore);
  llvm::Value *pStoreVal = pStore->getValueOperand();
  llvm::Value *pLoadVecAlloca = IRB.CreateLoad(pVecAlloca->getAllocatedType(), pVecAlloca);
  llvm::Value *pIns = pLoadVecAlloca;
  IGC_ASSERT(nullptr != pStoreVal);
  IGC_ASSERT(nullptr != pStoreVal->getType());
  if (pStoreVal->getType()->isVectorTy()) {
    // A vector store
    // store <2 x float> %v, <2 x float>* %ptr
    // becomes
    // %w = load <32 x float> *%ptr1
    // %v0 = extractelement <2 x float> %v, i32 0
    // %w0 = insertelement <32 x float> %w, float %v0, i32 %idx
    // %v1 = extractelement <2 x float> %v, i32 1
    // %w1 = insertelement <32 x float> %w0, float %v1, i32 %idx+1
    // store <32 x float> %w1, <32 x float>* %ptr1
    for (unsigned i = 0, e = (unsigned)cast<IGCLLVM::FixedVectorType>(pStoreVal->getType())->getNumElements(); i < e;
         ++i) {
      Value *VectorIdx = ConstantInt::get(pScalarizedIdx->getType(), i);
      auto Val = IRB.CreateExtractElement(pStoreVal, VectorIdx);
      Val = IRB.CreateBitCast(Val, pLoadVecAlloca->getType()->getScalarType());
      auto Idx = IRB.CreateAdd(pScalarizedIdx, VectorIdx);
      pIns = IRB.CreateInsertElement(pIns, Val, Idx);
    }
  } else {
    pStoreVal = IRB.CreateBitCast(pStoreVal, pLoadVecAlloca->getType()->getScalarType());
    pIns = IRB.CreateInsertElement(pLoadVecAlloca, pStoreVal, pScalarizedIdx);
  }
  IRB.CreateStore(pIns, pVecAlloca);
  pStore->eraseFromParent();
}

void TransposeHelperPromote::handleLifetimeMark(IntrinsicInst *inst) {
  IGC_ASSERT(nullptr != inst);
  IGC_ASSERT((inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) ||
             (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_end));
  if (inst->getIntrinsicID() == llvm::Intrinsic::lifetime_start) {
    pStartPoints.insert(inst);
  }
  m_toBeRemovedGEP.push_back(inst);
}
