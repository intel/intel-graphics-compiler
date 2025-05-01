/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenX.h"
#include "GenXRegionUtils.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Region.h"

#include "llvmWrapper/IR/Instructions.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#define DEBUG_TYPE "genx-legacy-to-lsc-translator"

using namespace llvm;
using namespace genx;

namespace {
class GenXLegacyToLscTranslator
    : public FunctionPass,
      public InstVisitor<GenXLegacyToLscTranslator> {
public:
  static char ID;
  explicit GenXLegacyToLscTranslator() : FunctionPass(ID) {}

  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

private:
  bool isLocal(Value *BTI) const;

  Value *translateOWordLoadStore(CallInst &CI) const;
  Value *translateGatherScatter(CallInst &CI) const;
  Value *translateSVMGatherScatter(CallInst &CI) const;
  Value *translateQuadGatherScatter(CallInst &CI) const;
  Value *translateAtomic(CallInst &CI) const;
  Value *translateMediaLoadStore(CallInst &CI) const;

  const GenXSubtarget *ST = nullptr;
};

char GenXLegacyToLscTranslator::ID = 0;

LSC_DATA_ELEMS getLSCElementsPerAddress(unsigned N) {
  switch (N) {
  case 1:
    return LSC_DATA_ELEMS_1;
  case 2:
    return LSC_DATA_ELEMS_2;
  case 3:
    return LSC_DATA_ELEMS_3;
  case 4:
    return LSC_DATA_ELEMS_4;
  case 8:
    return LSC_DATA_ELEMS_8;
  case 16:
    return LSC_DATA_ELEMS_16;
  case 32:
    return LSC_DATA_ELEMS_32;
  case 64:
    return LSC_DATA_ELEMS_64;
  default:
    IGC_ASSERT_UNREACHABLE();
    break;
  }
  return LSC_DATA_ELEMS_INVALID;
}

LSC_DATA_SIZE getLSCElementSize(unsigned ESize) {
  switch (ESize) {
  case ByteBytes:
    return LSC_DATA_SIZE_8c32b;
  case WordBytes:
    return LSC_DATA_SIZE_16c32b;
  case DWordBytes:
    return LSC_DATA_SIZE_32b;
  case QWordBytes:
    return LSC_DATA_SIZE_64b;
  default:
    IGC_ASSERT_UNREACHABLE();
    break;
  }
  return LSC_DATA_SIZE_INVALID;
}
} // namespace

namespace llvm {
void initializeGenXLegacyToLscTranslatorPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXLegacyToLscTranslator, "GenXLegacyToLscTranslator",
                      "GenXLegacyToLscTranslator", false, false)
INITIALIZE_PASS_END(GenXLegacyToLscTranslator, "GenXLegacyToLscTranslator",
                    "GenXLegacyToLscTranslator", false, false)

FunctionPass *llvm::createGenXLegacyToLscTranslatorPass() {
  initializeGenXLegacyToLscTranslatorPass(*PassRegistry::getPassRegistry());
  return new GenXLegacyToLscTranslator;
}

StringRef GenXLegacyToLscTranslator::getPassName() const {
  return "GenX legacy-to-lsc intrinsics translator";
}

void GenXLegacyToLscTranslator::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXBackendConfig>();
  AU.setPreservesCFG();
}

bool GenXLegacyToLscTranslator::runOnFunction(Function &F) {
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  IGC_ASSERT(ST);

  if (!ST->translateLegacyMessages())
    return false;

  visit(F);

  return true;
}

void GenXLegacyToLscTranslator::visitCallInst(CallInst &CI) {
  auto IID = vc::getAnyIntrinsicID(&CI);

  Value *NewCI = nullptr;

  switch (IID) {
  default:
    return;
  case GenXIntrinsic::genx_dword_atomic2_add:
  case GenXIntrinsic::genx_dword_atomic2_and:
  case GenXIntrinsic::genx_dword_atomic2_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic2_dec:
  case GenXIntrinsic::genx_dword_atomic2_imax:
  case GenXIntrinsic::genx_dword_atomic2_imin:
  case GenXIntrinsic::genx_dword_atomic2_inc:
  case GenXIntrinsic::genx_dword_atomic2_max:
  case GenXIntrinsic::genx_dword_atomic2_min:
  case GenXIntrinsic::genx_dword_atomic2_or:
  case GenXIntrinsic::genx_dword_atomic2_sub:
  case GenXIntrinsic::genx_dword_atomic2_xchg:
  case GenXIntrinsic::genx_dword_atomic2_xor:
  case GenXIntrinsic::genx_dword_atomic_add:
  case GenXIntrinsic::genx_dword_atomic_and:
  case GenXIntrinsic::genx_dword_atomic_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic_dec:
  case GenXIntrinsic::genx_dword_atomic_imax:
  case GenXIntrinsic::genx_dword_atomic_imin:
  case GenXIntrinsic::genx_dword_atomic_inc:
  case GenXIntrinsic::genx_dword_atomic_max:
  case GenXIntrinsic::genx_dword_atomic_min:
  case GenXIntrinsic::genx_dword_atomic_or:
  case GenXIntrinsic::genx_dword_atomic_sub:
  case GenXIntrinsic::genx_dword_atomic_xchg:
  case GenXIntrinsic::genx_dword_atomic_xor:
  case GenXIntrinsic::genx_svm_atomic_add:
  case GenXIntrinsic::genx_svm_atomic_and:
  case GenXIntrinsic::genx_svm_atomic_cmpxchg:
  case GenXIntrinsic::genx_svm_atomic_dec:
  case GenXIntrinsic::genx_svm_atomic_imax:
  case GenXIntrinsic::genx_svm_atomic_imin:
  case GenXIntrinsic::genx_svm_atomic_inc:
  case GenXIntrinsic::genx_svm_atomic_max:
  case GenXIntrinsic::genx_svm_atomic_min:
  case GenXIntrinsic::genx_svm_atomic_or:
  case GenXIntrinsic::genx_svm_atomic_sub:
  case GenXIntrinsic::genx_svm_atomic_xchg:
  case GenXIntrinsic::genx_svm_atomic_xor:
    NewCI = translateAtomic(CI);
    break;
  case GenXIntrinsic::genx_gather_scaled:
  case GenXIntrinsic::genx_gather_scaled2:
  case GenXIntrinsic::genx_gather_masked_scaled2:
  case GenXIntrinsic::genx_scatter_scaled:
    NewCI = translateGatherScatter(CI);
    break;
  case GenXIntrinsic::genx_svm_gather:
  case GenXIntrinsic::genx_svm_scatter:
    NewCI = translateSVMGatherScatter(CI);
    break;
  case GenXIntrinsic::genx_gather4_scaled:
  case GenXIntrinsic::genx_gather4_scaled2:
  case GenXIntrinsic::genx_gather4_masked_scaled2:
  case GenXIntrinsic::genx_scatter4_scaled:
  case GenXIntrinsic::genx_svm_gather4_scaled:
  case GenXIntrinsic::genx_svm_scatter4_scaled:
    NewCI = translateQuadGatherScatter(CI);
    break;
  case GenXIntrinsic::genx_oword_ld:
  case GenXIntrinsic::genx_oword_ld_unaligned:
  case GenXIntrinsic::genx_oword_st:
  case GenXIntrinsic::genx_svm_block_ld:
  case GenXIntrinsic::genx_svm_block_ld_unaligned:
  case GenXIntrinsic::genx_svm_block_st:
    NewCI = translateOWordLoadStore(CI);
    break;
  case GenXIntrinsic::genx_media_ld:
  case GenXIntrinsic::genx_media_st:
    if (!ST->translateMediaBlockMessages())
      return;
    NewCI = translateMediaLoadStore(CI);
    break;
  }

  if (!NewCI) {
    vc::diagnose(CI.getContext(), "GenXLegacyToLscTranslator",
                 "Cannot translate intrinsic", &CI);
    return;
  }

  if (!CI.getType()->isVoidTy()) {
    CI.replaceAllUsesWith(NewCI);
    NewCI->takeName(&CI);
  }

  CI.eraseFromParent();
}

Value *GenXLegacyToLscTranslator::translateOWordLoadStore(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IRBuilder<> Builder(&CI);
  auto IID = vc::getAnyIntrinsicID(&CI);

  auto NewIID = vc::InternalIntrinsic::not_any_intrinsic;
  Value *Base = nullptr;
  Value *Addr = nullptr;
  Value *Data = nullptr;
  auto AddrSize = LSC_ADDR_SIZE_INVALID;

  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_oword_ld:
  case GenXIntrinsic::genx_oword_ld_unaligned:
    NewIID = vc::InternalIntrinsic::lsc_load_bti;
    AddrSize = LSC_ADDR_SIZE_32b;
    Base = CI.getArgOperand(1);
    Addr = CI.getArgOperand(2);
    if (isLocal(Base)) {
      NewIID = vc::InternalIntrinsic::lsc_load_slm;
      Base = Builder.getInt32(0);
    }
    if (IID == GenXIntrinsic::genx_oword_ld)
      Addr = Builder.CreateShl(Addr, Log2_32(OWordBytes));
    break;
  case GenXIntrinsic::genx_oword_st:
    NewIID = vc::InternalIntrinsic::lsc_store_bti;
    AddrSize = LSC_ADDR_SIZE_32b;
    Base = CI.getArgOperand(0);
    Addr = Builder.CreateShl(CI.getArgOperand(1), Log2_32(OWordBytes));
    Data = CI.getArgOperand(2);
    if (isLocal(Base)) {
      NewIID = vc::InternalIntrinsic::lsc_store_slm;
      Base = Builder.getInt32(0);
    }
    break;
  case GenXIntrinsic::genx_svm_block_ld:
  case GenXIntrinsic::genx_svm_block_ld_unaligned:
    NewIID = vc::InternalIntrinsic::lsc_load_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    Base = Builder.getInt64(0);
    Addr = CI.getArgOperand(0);
    break;
  case GenXIntrinsic::genx_svm_block_st:
    NewIID = vc::InternalIntrinsic::lsc_store_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    Base = Builder.getInt64(0);
    Addr = CI.getArgOperand(0);
    Data = CI.getArgOperand(1);
    break;
  }

  auto *OrigVTy = dyn_cast<IGCLLVM::FixedVectorType>(CI.getType());
  if (!OrigVTy) {
    IGC_ASSERT_EXIT(Data);
    OrigVTy = dyn_cast<IGCLLVM::FixedVectorType>(Data->getType());
  }
  IGC_ASSERT(OrigVTy);

  auto SizeBits = OrigVTy->getScalarSizeInBits() * OrigVTy->getNumElements();
  auto NumDWords = SizeBits / DWordBits;
  auto *NewVTy = IGCLLVM::FixedVectorType::get(Builder.getInt32Ty(), NumDWords);

  auto *PredTy = IGCLLVM::FixedVectorType::get(Builder.getInt1Ty(), 1);
  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Type *, 4> Types;
  if (!Data)
    Types.push_back(NewVTy);
  Types.push_back(PredTy);
  Types.push_back(CacheOpts->getType());
  Types.push_back(Addr->getType());
  if (Data)
    Types.push_back(NewVTy);

  Data = Data ? Builder.CreateBitCast(Data, NewVTy) : UndefValue::get(NewVTy);
  SmallVector<Value *, 11> Args = {
      Constant::getAllOnesValue(PredTy),
      Builder.getInt8(AddrSize),
      Builder.getInt8(LSC_DATA_SIZE_32b),
      Builder.getInt8(getLSCElementsPerAddress(NumDWords)),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Data,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(CI.getModule(),
                                                             NewIID, Types);
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);

  if (I->getType()->isVoidTy())
    return I;

  return Builder.CreateBitCast(I, OrigVTy);
}

Value *GenXLegacyToLscTranslator::translateGatherScatter(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IRBuilder<> Builder(&CI);
  auto IID = vc::getAnyIntrinsicID(&CI);

  auto NewIID = vc::InternalIntrinsic::not_any_intrinsic;
  int PredIndex = -1;
  int NumBlocksIndex = -1;
  int BTIIndex = -1;
  int GlobalOffsetIndex = -1;
  int AddressIndex = -1;
  int SourceIndex = -1;
  bool IsLoad = true;

  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_scatter_scaled:
    IsLoad = false;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_gather_scaled:
    NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_bti
                    : vc::InternalIntrinsic::lsc_store_bti;
    PredIndex = 0;
    NumBlocksIndex = 1;
    BTIIndex = 3;
    GlobalOffsetIndex = 4;
    AddressIndex = 5;
    SourceIndex = 6;
    break;
  case GenXIntrinsic::genx_gather_masked_scaled2:
    PredIndex = 5;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_gather_scaled2:
    NumBlocksIndex = 0;
    BTIIndex = 2;
    GlobalOffsetIndex = 3;
    AddressIndex = 4;
    NewIID = vc::InternalIntrinsic::lsc_load_bti;
    break;
  }

  Value *Base = CI.getArgOperand(BTIIndex);
  if (isLocal(Base)) {
    Base = Builder.getInt32(0);
    NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_slm
                    : vc::InternalIntrinsic::lsc_store_slm;
  }

  auto *Addr = CI.getArgOperand(AddressIndex);
  auto *AddrVTy = cast<IGCLLVM::FixedVectorType>(Addr->getType());
  auto SimdWidth = AddrVTy->getNumElements();

  if (GlobalOffsetIndex >= 0) {
    auto *Offset = CI.getArgOperand(GlobalOffsetIndex);
    auto *Splat = Builder.CreateVectorSplat(SimdWidth, Offset);
    Addr = Builder.CreateAdd(Addr, Splat);
  }

  auto *PredVTy = IGCLLVM::FixedVectorType::get(Builder.getInt1Ty(), SimdWidth);
  Value *Pred = Constant::getAllOnesValue(PredVTy);
  if (PredIndex >= 0)
    Pred = CI.getArgOperand(PredIndex);
  IGC_ASSERT(Pred->getType() == PredVTy);

  auto *DataVTy =
      IGCLLVM::FixedVectorType::get(Builder.getInt32Ty(), SimdWidth);
  Value *Data = UndefValue::get(DataVTy);
  if (SourceIndex >= 0)
    Data = Builder.CreateBitCast(CI.getArgOperand(SourceIndex), DataVTy);

  auto *NumBlocks = cast<ConstantInt>(CI.getArgOperand(NumBlocksIndex));
  auto ElementSize = getLSCElementSize(1 << NumBlocks->getZExtValue());

  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Type *, 4> Types;
  if (IsLoad)
    Types.push_back(DataVTy);
  Types.push_back(PredVTy);
  Types.push_back(CacheOpts->getType());
  Types.push_back(AddrVTy);
  if (!IsLoad)
    Types.push_back(DataVTy);

  SmallVector<Value *, 10> Args = {
      Pred,
      Builder.getInt8(LSC_ADDR_SIZE_32b),
      Builder.getInt8(ElementSize),
      Builder.getInt8(LSC_DATA_ELEMS_1),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Data,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(CI.getModule(),
                                                             NewIID, Types);
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);

  if (I->getType()->isVoidTy())
    return I;

  return Builder.CreateBitCast(I, CI.getType());
}

Value *
GenXLegacyToLscTranslator::translateSVMGatherScatter(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IRBuilder<> Builder(&CI);

  auto IID = vc::getAnyIntrinsicID(&CI);
  IGC_ASSERT(IID == GenXIntrinsic::genx_svm_gather ||
             IID == GenXIntrinsic::genx_svm_scatter);
  bool IsLoad = IID == GenXIntrinsic::genx_svm_gather;
  auto NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_ugm
                       : vc::InternalIntrinsic::lsc_store_ugm;

  auto *Pred = CI.getArgOperand(0);
  auto *Log2NumBlocks = cast<ConstantInt>(CI.getArgOperand(1));
  auto *Addr = CI.getArgOperand(2);
  auto *Data = CI.getArgOperand(3);

  auto *PredVTy = cast<IGCLLVM::FixedVectorType>(Pred->getType());
  auto *AddrVTy = cast<IGCLLVM::FixedVectorType>(Addr->getType());

  auto SimdWidth = PredVTy->getNumElements();
  IGC_ASSERT(AddrVTy->getNumElements() == SimdWidth);

  auto *OrigVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto NumBlocks = 1 << Log2NumBlocks->getZExtValue();

  auto ElementBytes = OrigVTy->getScalarSizeInBits() / ByteBits;
  auto ElementSize = LSC_DATA_SIZE_INVALID;
  auto ElementsPerLane = 0;

  if (ElementBytes < DWordBytes) {
    ElementSize = getLSCElementSize(NumBlocks);
    ElementsPerLane = 1;
  } else {
    ElementSize = getLSCElementSize(ElementBytes);
    ElementsPerLane = NumBlocks;
  }

  auto *ETy = ElementSize == LSC_DATA_SIZE_64b ? Builder.getInt64Ty()
                                               : Builder.getInt32Ty();
  auto *VTy = IGCLLVM::FixedVectorType::get(ETy, SimdWidth * ElementsPerLane);
  Data = Builder.CreateBitCast(Data, VTy);

  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Type *, 4> Types;
  if (IsLoad)
    Types.push_back(VTy);
  Types.push_back(PredVTy);
  Types.push_back(CacheOpts->getType());
  Types.push_back(AddrVTy);
  if (!IsLoad)
    Types.push_back(VTy);

  SmallVector<Value *, 10> Args = {
      Pred,
      Builder.getInt8(LSC_ADDR_SIZE_64b),
      Builder.getInt8(ElementSize),
      Builder.getInt8(getLSCElementsPerAddress(ElementsPerLane)),
      CacheOpts,
      Builder.getInt64(0), // Address base
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Data,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(CI.getModule(),
                                                             NewIID, Types);
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);

  if (I->getType()->isVoidTy())
    return I;

  return Builder.CreateBitCast(I, CI.getType());
}

Value *
GenXLegacyToLscTranslator::translateQuadGatherScatter(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IRBuilder<> Builder(&CI);
  auto IID = vc::getAnyIntrinsicID(&CI);

  auto NewIID = vc::InternalIntrinsic::not_any_intrinsic;
  int PredIndex = -1;
  int ChannelMaskIndex = -1;
  int BTIIndex = -1;
  int GlobalOffsetIndex = -1;
  int AddressIndex = -1;
  int SourceIndex = -1;
  auto AddrSize = LSC_ADDR_SIZE_INVALID;
  bool IsLoad = true;

  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_scatter4_scaled:
    IsLoad = false;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_gather4_scaled:
    NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_quad_bti
                    : vc::InternalIntrinsic::lsc_store_quad_bti;
    AddrSize = LSC_ADDR_SIZE_32b;
    PredIndex = 0;
    ChannelMaskIndex = 1;
    BTIIndex = 3;
    GlobalOffsetIndex = 4;
    AddressIndex = 5;
    SourceIndex = 6;
    break;
  case GenXIntrinsic::genx_gather4_masked_scaled2:
    PredIndex = 5;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_gather4_scaled2:
    NewIID = vc::InternalIntrinsic::lsc_load_quad_bti;
    AddrSize = LSC_ADDR_SIZE_32b;
    ChannelMaskIndex = 0;
    BTIIndex = 2;
    GlobalOffsetIndex = 3;
    AddressIndex = 4;
    break;
  case GenXIntrinsic::genx_svm_scatter4_scaled:
    IsLoad = false;
    LLVM_FALLTHROUGH;
  case GenXIntrinsic::genx_svm_gather4_scaled:
    NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_quad_ugm
                    : vc::InternalIntrinsic::lsc_store_quad_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    PredIndex = 0;
    ChannelMaskIndex = 1;
    GlobalOffsetIndex = 3;
    AddressIndex = 4;
    SourceIndex = 5;
    break;
  }

  Value *Base =
      BTIIndex >= 0 ? CI.getArgOperand(BTIIndex) : Builder.getInt64(0);
  if (isLocal(Base)) {
    Base = Builder.getInt32(0);
    NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_quad_slm
                    : vc::InternalIntrinsic::lsc_store_quad_slm;
  }

  auto *Addr = CI.getArgOperand(AddressIndex);
  auto *AddrVTy = cast<IGCLLVM::FixedVectorType>(Addr->getType());
  auto SimdWidth = AddrVTy->getNumElements();

  if (GlobalOffsetIndex >= 0) {
    auto *Offset = CI.getArgOperand(GlobalOffsetIndex);
    auto *Splat = Builder.CreateVectorSplat(SimdWidth, Offset);
    Addr = Builder.CreateAdd(Addr, Splat);
  }

  auto *PredVTy = IGCLLVM::FixedVectorType::get(Builder.getInt1Ty(), SimdWidth);
  Value *Pred = Constant::getAllOnesValue(PredVTy);
  if (PredIndex >= 0)
    Pred = CI.getArgOperand(PredIndex);
  IGC_ASSERT(Pred->getType() == PredVTy);

  IGC_ASSERT(ChannelMaskIndex >= 0);
  auto ChannelMask =
      cast<ConstantInt>(CI.getArgOperand(ChannelMaskIndex))->getZExtValue();
  auto NumChannels = countPopulation(ChannelMask);

  Type *DataVTy = nullptr;
  Value *Data = nullptr;
  unsigned OrigWidth = 0;
  unsigned NewWidth = SimdWidth * NumChannels;

  if (SourceIndex >= 0) {
    Data = CI.getArgOperand(SourceIndex);
    auto *OrigDataVTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
    Type *ETy = OrigDataVTy->getElementType();
    DataVTy = IGCLLVM::FixedVectorType::get(ETy, NewWidth);
    OrigWidth = OrigDataVTy->getNumElements();
    if (OrigWidth > NewWidth) {
      Region RdR(Data);
      RdR.Width = RdR.NumElements = NewWidth;
      RdR.Offset = 0;
      Data = RdR.createRdRegion(Data, "", &CI, CI.getDebugLoc());
    }
  } else {
    auto *RetVTy = cast<IGCLLVM::FixedVectorType>(CI.getType());
    Type *ETy = RetVTy->getElementType();
    DataVTy = IGCLLVM::FixedVectorType::get(ETy, NewWidth);
    Data = UndefValue::get(DataVTy);
    OrigWidth = RetVTy->getNumElements();
  }

  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Type *, 4> Types;
  if (IsLoad)
    Types.push_back(DataVTy);
  Types.push_back(PredVTy);
  Types.push_back(CacheOpts->getType());
  Types.push_back(AddrVTy);
  if (!IsLoad)
    Types.push_back(DataVTy);

  SmallVector<Value *, 10> Args = {
      Pred,
      Builder.getInt8(AddrSize),
      Builder.getInt8(LSC_DATA_SIZE_32b),
      Builder.getInt8(ChannelMask),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Data,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(CI.getModule(),
                                                             NewIID, Types);
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);

  if (IsLoad && OrigWidth > NewWidth) {
    Region WrR(&CI);
    WrR.Width = WrR.NumElements = NewWidth;
    WrR.Offset = 0;
    WrR.Mask = nullptr;
    return WrR.createWrRegion(UndefValue::get(CI.getType()), I, "", &CI,
                              CI.getDebugLoc());
  }

  return I;
}

Value *GenXLegacyToLscTranslator::translateAtomic(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IRBuilder<> Builder(&CI);
  auto IID = vc::getAnyIntrinsicID(&CI);

  int Opcode = -1;
  unsigned NumSrcs = 1;
  switch (IID) {
  default:
    IGC_ASSERT_UNREACHABLE();
  case GenXIntrinsic::genx_dword_atomic2_add:
  case GenXIntrinsic::genx_dword_atomic_add:
  case GenXIntrinsic::genx_svm_atomic_add:
    Opcode = LSC_ATOMIC_IADD;
    break;
  case GenXIntrinsic::genx_dword_atomic2_and:
  case GenXIntrinsic::genx_dword_atomic_and:
  case GenXIntrinsic::genx_svm_atomic_and:
    Opcode = LSC_ATOMIC_AND;
    break;
  case GenXIntrinsic::genx_dword_atomic2_cmpxchg:
  case GenXIntrinsic::genx_dword_atomic_cmpxchg:
  case GenXIntrinsic::genx_svm_atomic_cmpxchg:
    Opcode = LSC_ATOMIC_ICAS;
    NumSrcs = 2;
    break;
  case GenXIntrinsic::genx_dword_atomic2_dec:
  case GenXIntrinsic::genx_dword_atomic_dec:
  case GenXIntrinsic::genx_svm_atomic_dec:
    Opcode = LSC_ATOMIC_IDEC;
    NumSrcs = 0;
    break;
  case GenXIntrinsic::genx_dword_atomic2_imax:
  case GenXIntrinsic::genx_dword_atomic_imax:
  case GenXIntrinsic::genx_svm_atomic_imax:
    Opcode = LSC_ATOMIC_SMAX;
    break;
  case GenXIntrinsic::genx_dword_atomic2_imin:
  case GenXIntrinsic::genx_dword_atomic_imin:
  case GenXIntrinsic::genx_svm_atomic_imin:
    Opcode = LSC_ATOMIC_SMIN;
    break;
  case GenXIntrinsic::genx_dword_atomic2_inc:
  case GenXIntrinsic::genx_dword_atomic_inc:
  case GenXIntrinsic::genx_svm_atomic_inc:
    Opcode = LSC_ATOMIC_IINC;
    NumSrcs = 0;
    break;
  case GenXIntrinsic::genx_dword_atomic2_max:
  case GenXIntrinsic::genx_dword_atomic_max:
  case GenXIntrinsic::genx_svm_atomic_max:
    Opcode = LSC_ATOMIC_UMAX;
    break;
  case GenXIntrinsic::genx_dword_atomic2_min:
  case GenXIntrinsic::genx_dword_atomic_min:
  case GenXIntrinsic::genx_svm_atomic_min:
    Opcode = LSC_ATOMIC_UMIN;
    break;
  case GenXIntrinsic::genx_dword_atomic2_or:
  case GenXIntrinsic::genx_dword_atomic_or:
  case GenXIntrinsic::genx_svm_atomic_or:
    Opcode = LSC_ATOMIC_OR;
    break;
  case GenXIntrinsic::genx_dword_atomic2_sub:
  case GenXIntrinsic::genx_dword_atomic_sub:
  case GenXIntrinsic::genx_svm_atomic_sub:
    Opcode = LSC_ATOMIC_ISUB;
    break;
  case GenXIntrinsic::genx_dword_atomic2_xchg:
  case GenXIntrinsic::genx_dword_atomic_xchg:
  case GenXIntrinsic::genx_svm_atomic_xchg:
    Opcode = LSC_ATOMIC_STORE;
    break;
  case GenXIntrinsic::genx_dword_atomic2_xor:
  case GenXIntrinsic::genx_dword_atomic_xor:
  case GenXIntrinsic::genx_svm_atomic_xor:
    Opcode = LSC_ATOMIC_XOR;
    break;
  }

  auto *Ty = CI.getType();
  auto AddrSize = LSC_ADDR_SIZE_32b;
  auto DataSize =
      Ty->isIntOrIntVectorTy(64) ? LSC_DATA_SIZE_64b : LSC_DATA_SIZE_32b;

  int ArgN = 0;
  auto *Pred = CI.getArgOperand(ArgN++);
  auto *Base = CI.getArgOperand(ArgN++);

  auto NewIID = vc::InternalIntrinsic::lsc_atomic_bti;

  if (isa<IGCLLVM::FixedVectorType>(Base->getType())) {
    // SVM atomics have no BTI/base
    NewIID = vc::InternalIntrinsic::lsc_atomic_ugm;
    AddrSize = LSC_ADDR_SIZE_64b;
    Base = Builder.getInt64(0);
    ArgN -= 1;
  } else if (isLocal(Base)) {
    NewIID = vc::InternalIntrinsic::lsc_atomic_slm;
    Base = Builder.getInt32(0);
  }

  auto *Addr = CI.getArgOperand(ArgN++);
  Value *Src0 = UndefValue::get(Ty);
  Value *Src1 = UndefValue::get(Ty);
  Value *Passthru = UndefValue::get(Ty);

  if (NumSrcs == 1) {
    Src0 = CI.getArgOperand(ArgN++);
  } else if (NumSrcs == 2) {
    Src1 = CI.getArgOperand(ArgN++);
    Src0 = CI.getArgOperand(ArgN++);
  }

  if (IGCLLVM::getNumArgOperands(&CI) == ArgN + 1)
    Passthru = CI.getArgOperand(ArgN);

  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Value *, 12> Args = {
      Pred,
      Builder.getInt8(Opcode),
      Builder.getInt8(AddrSize),
      Builder.getInt8(DataSize),
      CacheOpts,
      Base,
      Addr,
      Builder.getInt16(1), // Address scale
      Builder.getInt32(0), // Address offset
      Src0,
      Src1,
      Passthru,
  };

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      CI.getModule(), NewIID,
      {Ty, Pred->getType(), CacheOpts->getType(), Addr->getType()});
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);
  return I;
}

Value *GenXLegacyToLscTranslator::translateMediaLoadStore(CallInst &CI) const {
  LLVM_DEBUG(dbgs() << "Translate intrinsic: " << CI);
  IGC_ASSERT(ST->translateMediaBlockMessages());

  IRBuilder<> Builder(&CI);
  auto IID = vc::getAnyIntrinsicID(&CI);

  IGC_ASSERT(IID == GenXIntrinsic::genx_media_ld ||
             IID == GenXIntrinsic::genx_media_st);
  auto IsLoad = IID == GenXIntrinsic::genx_media_ld;
  auto NewIID = IsLoad ? vc::InternalIntrinsic::lsc_load_2d_tgm_bti
                       : vc::InternalIntrinsic::lsc_store_2d_tgm_bti;

  auto *Modifier = cast<ConstantInt>(CI.getArgOperand(0));
  auto *BTI = CI.getArgOperand(1);
  auto *Plane = cast<ConstantInt>(CI.getArgOperand(2));
  auto *BlockWidth = cast<ConstantInt>(CI.getArgOperand(3));
  auto *AddrX = CI.getArgOperand(4);
  auto *AddrY = CI.getArgOperand(5);
  Value *Data = nullptr;
  IGCLLVM::FixedVectorType *VTy = nullptr;

  if (IsLoad) {
    VTy = cast<IGCLLVM::FixedVectorType>(CI.getType());
  } else {
    Data = CI.getArgOperand(6);
    VTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  }

  if (Modifier->getZExtValue() != 0) {
    LLVM_DEBUG(dbgs() << "Modifiers are not supported for media block "
                         "intrinsic translations: "
                      << CI);
    return nullptr;
  }
  if (Plane->getZExtValue() != 0) {
    LLVM_DEBUG(dbgs() << "Non-zero plane is not supported for media block "
                         "intrinsic translations: "
                      << CI);
    return nullptr;
  }

  auto *ETy = VTy->getElementType();
  unsigned ESize = ETy->getScalarSizeInBits() / ByteBits;
  auto DataSize = ESize * VTy->getNumElements();

  unsigned Width = BlockWidth->getZExtValue();
  unsigned RoundedWidth = roundedVal(Width, 4u);
  unsigned Height = DataSize / RoundedWidth;
  IGC_ASSERT(Width > 0 && Width <= 64);
  IGC_ASSERT(Width % ESize == 0);
  IGC_ASSERT(DataSize % RoundedWidth == 0);

  auto *CacheOpts = ConstantDataVector::getSplat(
      ST->getNumCacheLevels(), Builder.getInt8(LSC_CACHING_DEFAULT));

  SmallVector<Type *, 2> Types;
  if (IsLoad)
    Types.push_back(VTy);
  Types.push_back(CacheOpts->getType());
  if (!IsLoad)
    Types.push_back(VTy);

  SmallVector<Value *, 8> Args = {
      CacheOpts,
      BTI,
      Builder.getInt32(Height),
      Builder.getInt32(Width / ESize),
      AddrX,
      AddrY,
  };
  if (!IsLoad)
    Args.push_back(Data);

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(CI.getModule(),
                                                             NewIID, Types);
  auto *I = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "New intrinsic generated: " << *I);
  return I;
}

bool GenXLegacyToLscTranslator::isLocal(Value *BTI) const {
  if (auto *C = dyn_cast<ConstantInt>(BTI))
    return C->getZExtValue() == visa::ReservedSurfaceIndex::RSI_Slm;
  return false;
}
