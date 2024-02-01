/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// GenXLoadStoreLegalization
/// -------------------------
///
/// SIMD loads, stores, atomics and prefetches are considered legal if they
/// satisfy the following requirements:
///
/// 1. ExecSize is a power of 2 up to a device-specific MaxExecSize
/// 2. Operations with VectorSize > 1 must have DataType D32 or D64.
/// 3. The number of registers used by the data payload is no more than 8:
///    ceil(ExecSize * DataSize / GRFSize) * VectorSize <= 8
///    Here, ExecSize = max(ExecSize, MaxExecSize / 2).
///    That is, a single channel of data takes up at least 1 register if
///    DataType is D32 and at least 2 registers if DataType is D64. This
///    effectively means that the maximum legal VectorSize is 8 if DataType is
///    D32 and 4 if DataType is D64.
///
/// If a non-transposed operation doesn't obey rules 1 or 3, we make it legal
/// by splitting it into several parts. Data is loaded and stored
/// in Structure of Arrays format, so splitting requires some care.
/// If the instruction doesn't obey rule 2 or its VectorSize is larger than the
/// largest one allowed for its DataType, it can't be made legal, so we raise an
/// error.
///
/// SIMD1 Block loads, stores and prefetches are considered legal
/// if they satisfy the following requirements:
///
/// 1. DataType is either D32 or D64.
/// 2. The number of registers used by the data payload is no more than 8:
///    ceil(VectorSize * DataSize / GRFSize) <= 8
///

#include "GenX.h"
#include "GenXRegionUtils.h"
#include "GenXSubtarget.h"
#include "GenXTargetMachine.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Support/GenXDiagnostic.h"
#include "vc/Utils/GenX/Region.h"

#include <llvm/CodeGen/TargetPassConfig.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Pass.h>

#include <numeric>

#define DEBUG_TYPE "genx-load-store-legalization"

using namespace llvm;
using namespace genx;

namespace {
class GenXLoadStoreLegalization
    : public FunctionPass,
      public InstVisitor<GenXLoadStoreLegalization> {
public:
  static char ID;
  explicit GenXLoadStoreLegalization() : FunctionPass(ID) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
  bool runOnFunction(Function &F) override;

  void visitCallInst(CallInst &CI);

private:
  unsigned getMaxLegalSimdWidth(CallInst &CI) const;
  unsigned getMinLegalSimdWidth(CallInst &CI) const;

  Value *splitMemoryOperation(Value *InsertTo, CallInst &CI,
                              unsigned SplitWidth, unsigned &Index) const;
  Value *extendMemoryOperation(Value *InsertTo, CallInst &CI,
                               unsigned ExtendWidth, unsigned Index) const;

  Function *getMemoryIntrinsic(CallInst &CI, unsigned NewWidth) const;

  Value *createExtractFromSOAValue(Instruction *InsertBefore, Value *Data,
                                   unsigned VectorSize, unsigned Index,
                                   unsigned Width) const;
  Value *createInsertToSOAValue(Instruction *InsertBefore, Value *InsertTo,
                                Value *Data, unsigned VectorSize,
                                unsigned Index, unsigned Width) const;

  const GenXSubtarget *ST = nullptr;
};

char GenXLoadStoreLegalization::ID = 0;
} // namespace

namespace llvm {
void initializeGenXLoadStoreLegalizationPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXLoadStoreLegalization, "GenXLoadStoreLegalization",
                      "GenXLoadStoreLegalization", false, false)
INITIALIZE_PASS_END(GenXLoadStoreLegalization, "GenXLoadStoreLegalization",
                    "GenXLoadStoreLegalization", false, false)

FunctionPass *llvm::createGenXLoadStoreLegalizationPass() {
  initializeGenXLoadStoreLegalizationPass(*PassRegistry::getPassRegistry());
  return new GenXLoadStoreLegalization;
}

StringRef GenXLoadStoreLegalization::getPassName() const {
  return "GenX load store legalization";
}

void GenXLoadStoreLegalization::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.addRequired<TargetPassConfig>();
  AU.addRequired<GenXBackendConfig>();
  AU.setPreservesCFG();
}

bool GenXLoadStoreLegalization::runOnFunction(Function &F) {
  ST = &getAnalysis<TargetPassConfig>()
            .getTM<GenXTargetMachine>()
            .getGenXSubtarget();
  IGC_ASSERT(ST);

  visit(F);

  return true;
}

void GenXLoadStoreLegalization::visitCallInst(CallInst &CI) {
  const auto IID = vc::InternalIntrinsic::getInternalIntrinsicID(&CI);
  if (!vc::InternalIntrinsic::isInternalMemoryIntrinsic(IID))
    return;
  LLVM_DEBUG(dbgs() << "Trying: " << CI << "\n");

  const auto ExecSize = vc::InternalIntrinsic::getMemorySimdWidth(&CI);
  const uint64_t VectorSize =
      vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);

  const auto MaxWidth = getMaxLegalSimdWidth(CI);
  const auto MinWidth = getMinLegalSimdWidth(CI);

  // No legalization needed: scalar/block operation or legal simd width
  if (vc::InternalIntrinsic::isMemoryBlockIntrinsic(&CI) ||
      (isPowerOf2_32(ExecSize) && ExecSize >= MinWidth &&
       ExecSize <= MaxWidth)) {
    LLVM_DEBUG(dbgs() << "The instruction is already legal: " << CI << "\n");
    return;
  }

  // Legalization is impossible, emit an error
  if (MaxWidth == 0 || MinWidth == 0) {
    vc::diagnose(CI.getContext(), "GenXLoadStoreLegalization",
                 "Cannot legalize illegal memory intrinsic", &CI);
    return;
  }

  auto *RetTy = CI.getType();
  Value *Ret = RetTy->isVoidTy() ? nullptr : UndefValue::get(RetTy);

  unsigned Index = 0;
  Ret = splitMemoryOperation(Ret, CI, MaxWidth, Index);

  unsigned RestSize = ExecSize - Index;
  if (RestSize != 0) {
    auto ExtWidth = std::max<unsigned>(PowerOf2Ceil(RestSize), MinWidth);
    Ret = extendMemoryOperation(Ret, CI, ExtWidth, Index);
  }

  if (Ret) {
    CI.replaceAllUsesWith(Ret);
    Ret->takeName(&CI);
  }
  CI.eraseFromParent();
}

Value *GenXLoadStoreLegalization::splitMemoryOperation(Value *InsertTo,
                                                       CallInst &CI,
                                                       unsigned SplitWidth,
                                                       unsigned &Index) const {
  const auto ExecSize = vc::InternalIntrinsic::getMemorySimdWidth(&CI);
  const uint64_t VectorSize =
      vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);

  auto *Func = getMemoryIntrinsic(CI, SplitWidth);
  IRBuilder<> Builder(&CI);

  for (; Index + SplitWidth <= ExecSize; Index += SplitWidth) {
    SmallVector<Value *, 13> Args;
    llvm::transform(CI.args(), std::back_inserter(Args), [&](Value *Arg) {
      auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Arg->getType());
      if (!VTy)
        return Arg;
      auto NumElements = VTy->getNumElements();
      bool IsSOA = NumElements == VectorSize * ExecSize;
      if (NumElements != ExecSize && !IsSOA)
        return Arg;
      return createExtractFromSOAValue(&CI, Arg, IsSOA ? VectorSize : 1, Index,
                                       SplitWidth);
    });

    auto *NewCI = Builder.CreateCall(Func, Args);
    LLVM_DEBUG(dbgs() << "Created split: " << *NewCI << "\n");

    if (InsertTo)
      InsertTo = createInsertToSOAValue(&CI, InsertTo, NewCI, VectorSize, Index,
                                        SplitWidth);
  }

  return InsertTo;
}

Value *GenXLoadStoreLegalization::extendMemoryOperation(Value *InsertTo,
                                                        CallInst &CI,
                                                        unsigned ExtendWidth,
                                                        unsigned Index) const {
  const auto ExecSize = vc::InternalIntrinsic::getMemorySimdWidth(&CI);
  const uint64_t VectorSize =
      vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);
  const auto RestSize = ExecSize - Index;
  if (RestSize == 0)
    return InsertTo;

  IGC_ASSERT(RestSize <= ExtendWidth);

  auto *Func = getMemoryIntrinsic(CI, ExtendWidth);
  IRBuilder<> Builder(&CI);

  SmallVector<Value *, 13> Args;
  llvm::transform(CI.args(), std::back_inserter(Args), [&](Value *Arg) {
    auto *VTy = dyn_cast<IGCLLVM::FixedVectorType>(Arg->getType());
    if (!VTy)
      return Arg;

    auto NumElements = VTy->getNumElements();
    bool IsSOA = NumElements == VectorSize * ExecSize;
    if (NumElements != ExecSize && !IsSOA)
      return Arg;

    if (Index > 0)
      Arg = createExtractFromSOAValue(&CI, Arg, IsSOA ? VectorSize : 1, Index,
                                      RestSize);
    if (RestSize == ExtendWidth)
      return Arg;

    auto *ETy = VTy->getElementType();
    auto *InsTy = IGCLLVM::FixedVectorType::get(
        ETy, IsSOA ? VectorSize * ExtendWidth : ExtendWidth);
    Value *Insert = ETy->isIntegerTy(1) ? Constant::getNullValue(InsTy)
                                        : UndefValue::get(InsTy);
    return createInsertToSOAValue(&CI, Insert, Arg, IsSOA ? VectorSize : 1, 0,
                                  RestSize);
  });

  Value *Res = Builder.CreateCall(Func, Args);
  LLVM_DEBUG(dbgs() << "Created extend: " << *Res << "\n");

  if (InsertTo) {
    if (RestSize != ExtendWidth)
      Res = createExtractFromSOAValue(&CI, Res, VectorSize, 0, RestSize);
    if (Index == 0)
      InsertTo = Res;
    else
      InsertTo = createInsertToSOAValue(&CI, InsertTo, Res, VectorSize, Index,
                                        RestSize);
  }

  return InsertTo;
}

Function *
GenXLoadStoreLegalization::getMemoryIntrinsic(CallInst &CI,
                                              unsigned NewWidth) const {
  const auto IID = vc::InternalIntrinsic::getInternalIntrinsicID(&CI);
  IGC_ASSERT_EXIT(IID != vc::InternalIntrinsic::not_internal_intrinsic);
  const uint64_t ExecSize = vc::InternalIntrinsic::getMemorySimdWidth(&CI);
  const uint64_t VectorSize =
      vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);

  SmallVector<Type *, 3> OverloadedTypes;
  if (vc::InternalIntrinsic::isOverloadedRet(IID)) {
    auto *ResVTy = cast<IGCLLVM::FixedVectorType>(CI.getType());
    IGC_ASSERT(ResVTy->getNumElements() == VectorSize * ExecSize);
    auto *ETy = ResVTy->getElementType();
    auto *VTy = IGCLLVM::FixedVectorType::get(ETy, VectorSize * NewWidth);
    OverloadedTypes.push_back(VTy);
  }

  for (unsigned I = 0; I < CI.getNumOperands(); I++) {
    if (!vc::InternalIntrinsic::isOverloadedArg(IID, I))
      continue;

    auto *Arg = CI.getOperand(I);
    auto *ArgTy = Arg->getType();
    if (!isa<IGCLLVM::FixedVectorType>(ArgTy)) {
      OverloadedTypes.push_back(ArgTy);
      continue;
    }

    auto *ArgVTy = cast<IGCLLVM::FixedVectorType>(ArgTy);
    auto *ETy = ArgVTy->getElementType();
    auto NumElements = ArgVTy->getNumElements();
    bool IsSOA = NumElements == VectorSize * ExecSize;

    if (NumElements != ExecSize && !IsSOA) {
      OverloadedTypes.push_back(ArgTy);
      continue;
    }

    auto Width = IsSOA ? VectorSize * NewWidth : NewWidth;
    auto *VTy = IGCLLVM::FixedVectorType::get(ETy, Width);
    OverloadedTypes.push_back(VTy);
  }

  auto *Func = vc::InternalIntrinsic::getInternalDeclaration(
      CI.getModule(), IID, OverloadedTypes);
  return Func;
}

unsigned GenXLoadStoreLegalization::getMaxLegalSimdWidth(CallInst &CI) const {
  IGC_ASSERT(vc::InternalIntrinsic::isInternalMemoryIntrinsic(&CI));
  // FIXME: support legacy memory intrinsics

  auto VectorSize = vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);
  auto ElementBytes =
      vc::InternalIntrinsic::getMemoryRegisterElementSize(&CI) / ByteBits;
  auto MaxWidth = ST->getLSCMaxWidth();
  auto MaxBytes = ST->getGRFByteSize() * ST->getLSCMaxDataRegisters();

  if (MaxBytes < MaxWidth * ElementBytes * VectorSize)
    return getMinLegalSimdWidth(CI);

  return MaxWidth;
}

unsigned GenXLoadStoreLegalization::getMinLegalSimdWidth(CallInst &CI) const {
  IGC_ASSERT(vc::InternalIntrinsic::isInternalMemoryIntrinsic(&CI));

  auto VectorSize = vc::InternalIntrinsic::getMemoryVectorSizePerLane(&CI);
  if (VectorSize == 1)
    return 1;

  // FIXME: support legacy memory intrinsics

  auto ElementBytes =
      vc::InternalIntrinsic::getMemoryRegisterElementSize(&CI) / ByteBits;
  auto MaxWidth = ST->getLSCMinWidth();
  auto MaxBytes = ST->getGRFByteSize() * ST->getLSCMaxDataRegisters();

  if (MaxBytes < MaxWidth * ElementBytes * VectorSize)
    return 0; // Invalid width

  return MaxWidth;
}

Value *GenXLoadStoreLegalization::createExtractFromSOAValue(
    Instruction *InsertBefore, Value *Data, unsigned VectorSize, unsigned Index,
    unsigned Width) const {
  auto *VTy = cast<IGCLLVM::FixedVectorType>(Data->getType());
  auto *ETy = VTy->getElementType();
  auto OrigWidth = VTy->getNumElements() / VectorSize;

  if (isa<UndefValue>(Data)) {
    auto *ResTy = IGCLLVM::FixedVectorType::get(ETy, Width * VectorSize);
    return UndefValue::get(ResTy);
  }

  Value *Extract = nullptr;
  const auto &DL = InsertBefore->getDebugLoc();

  if (ETy->isIntegerTy(1)) {
    IGC_ASSERT(VectorSize == 1);
    Extract = Region::createRdPredRegionOrConst(Data, Index, Width, "",
                                                InsertBefore, DL);
  } else {
    Region R(Data);
    R.Offset = Index * R.ElementBytes;
    R.VStride = OrigWidth;
    R.Width = Width;
    R.Stride = 1;
    R.NumElements = Width * VectorSize;
    Extract = R.createRdRegion(Data, "", InsertBefore, DL);
  }

  return Extract;
}

Value *GenXLoadStoreLegalization::createInsertToSOAValue(
    Instruction *InsertBefore, Value *InsertTo, Value *Data,
    unsigned VectorSize, unsigned Index, unsigned Width) const {
  auto *VTy = cast<IGCLLVM::FixedVectorType>(InsertTo->getType());
  auto OrigWidth = VTy->getNumElements() / VectorSize;

  Value *Insert = nullptr;
  const auto &DL = InsertBefore->getDebugLoc();

  if (VTy->getElementType()->isIntegerTy(1)) {
    IGC_ASSERT(VectorSize == 1);
    Insert =
        Region::createWrPredRegion(InsertTo, Data, Index, "", InsertBefore, DL);
  } else {
    Region R(InsertTo);
    R.Offset = Index * R.ElementBytes;
    R.VStride = OrigWidth;
    R.Width = Width;
    R.Stride = 1;
    R.NumElements = Width * VectorSize;
    Insert = R.createWrRegion(InsertTo, Data, "", InsertBefore, DL);
  }

  return Insert;
}
