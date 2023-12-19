/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXVerify / Regioning.
//===----------------------------------------------------------------------===//
//
// This file contains genx regioning intrinsics verification for GenXVerify
// pass.
//

#include "GenXVerify.h"

#include "llvmWrapper/IR/Instructions.h"

void GenXVerify::verifyRegioning(const CallInst &CI,
                                 const unsigned IntrinsicId) {
  auto ensureNumEls4_8_16_and_offsetIsAMultipleOf =
      [&](const auto NumElts, const Twine NumEltsArgDesc,
          unsigned OffsetOpndNum) -> void {
    if (Stage < GenXVerifyStage::PostGenXLegalization)
      return;
    if (!ensure(NumElts == 4 || NumElts == 8 || NumElts == 16,
                NumEltsArgDesc + " must be 4, 8 or 16", CI))
      return;
    const auto *OffsetInElementsConstInt =
        llvm::dyn_cast<llvm::ConstantInt>(CI.getOperand(OffsetOpndNum));
    if (!ensure(OffsetInElementsConstInt,
                "offset in elements must be a constant integer", CI))
      return;
    ensure(!(OffsetInElementsConstInt->getZExtValue() % NumElts),
           "offset in elements must be multiple of the number of elements of " +
               NumEltsArgDesc,
           CI);
  };

  size_t SrcOpndIdx = -1;
  switch (IntrinsicId) {
  case GenXIntrinsic::genx_rdregionf:
  case GenXIntrinsic::genx_rdregioni:
    if (!ensure(IGCLLVM::getNumArgOperands(&CI) == 6,
                "rdregion* intrinsics must have 6 operands", CI))
      return;
    SrcOpndIdx = 0;
    break;
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrconstregion:
    if (!ensure(IGCLLVM::getNumArgOperands(&CI) == 8,
                "wrregion* intrinsics must have 8 operands", CI))
      return;
    SrcOpndIdx = 1;
    break;
  case GenXIntrinsic::genx_rdpredregion: {
    if (!ensure(IGCLLVM::getNumArgOperands(&CI) == 2,
                "rdpredregion* intrinsics must have 2 operands", CI))
      return;
    const auto *RetVT = dyn_cast<IGCLLVM::FixedVectorType>(
        CI.getCalledFunction()->getReturnType());
    if (!ensure(RetVT, "return type must be a vector", CI))
      return;
    ensureNumEls4_8_16_and_offsetIsAMultipleOf(RetVT->getNumElements(),
                                               "returned vector", 1);
  }
    return;
  case GenXIntrinsic::genx_wrpredpredregion: {
    if (!ensure(IGCLLVM::getNumArgOperands(&CI) == 4,
                "wrpredpredregion* intrinsics must have 4 operands", CI))
      return;

    // The constant offset indexes both the vector itself and the predicate.
    // This intrinsic is valid only if the predicate is an EM value, and the
    // subvector operand is the result of a cmp (which is then baled in).
    ensure(isa<CmpInst>(CI.getOperand(1)),
           "subvector to write must be the direct result of a cmp instruction",
           CI);

    const auto *SubvectorToWriteT =
        dyn_cast<IGCLLVM::FixedVectorType>(CI.getOperand(1)->getType());

    if (!ensure(SubvectorToWriteT, "subvector to write must be a fixed vector",
                CI))
      return;
    ensureNumEls4_8_16_and_offsetIsAMultipleOf(
        SubvectorToWriteT->getNumElements(), "subvector to write", 2);

    // TODO: This intrinsic is valid only if the predicate is an EM value
    //       CI.getOperand(3)
  }
    return;
  case GenXIntrinsic::genx_wrpredregion: {
    if (!ensure(IGCLLVM::getNumArgOperands(&CI) == 3,
                "wrpredregion* intrinsics must have 3 operands", CI))
      return;

    const auto *SubvectorToWriteT =
        dyn_cast<IGCLLVM::FixedVectorType>(CI.getOperand(1)->getType());

    if (!ensure(SubvectorToWriteT, "subvector to write must be a fixed vector",
                CI))
      return;
    ensureNumEls4_8_16_and_offsetIsAMultipleOf(
        SubvectorToWriteT->getNumElements(), "subvector to write", 2);
  }
    return;
  default:
    IGC_ASSERT_MESSAGE(false, "This function must not be called with anything "
                              "except for regioning-related routines");
    return;
  };

  IGC_ASSERT(SrcOpndIdx != -1);

  const llvm::Value *SrcOpnd = CI.getOperand(SrcOpndIdx);
  const llvm::Type *SrcOpndT = CI.getOperand(SrcOpndIdx)->getType();
  const auto *SrcOpndMaybeVT = dyn_cast<IGCLLVM::FixedVectorType>(SrcOpndT);

  const llvm::Type *EltT = SrcOpndT->getScalarType();
  const unsigned EltSizeInBits = SrcOpndT->getScalarSizeInBits();

  ensure(
      EltT->getScalarType()->isFloatingPointTy() ||
          EltT->getScalarType()->isPointerTy() ||
          (EltT->getScalarType()->isIntegerTy() &&
           isPowerOf2_64(EltSizeInBits) && EltSizeInBits >= 8 &&
           EltSizeInBits <= 64),
      "The element type must be an integral power of two number of bytes up to "
      "and including 8 bytes in size, thus one of i8, i16, i32, i64, half, "
      "float, double. In particular i1 is not allowed.",
      CI);

  const llvm::Type *RetT = CI.getCalledFunction()->getReturnType();
  const llvm::Type *RetEltT = RetT->getScalarType();
  ensure(RetEltT == EltT,
         "The return type must be a vector with the same element type as the "
         "input "
         "vector, and number of elements giving the total size of the region. "
         "A scalar can be used instead of a 1-vector.",
         CI);

  const auto *RetTMaybeVT = dyn_cast<IGCLLVM::FixedVectorType>(RetT);
  // TODO:spec review: found non-vector cases.
  ensure(RetTMaybeVT, "The return type must be a vector.", CI, IsFatal::No);
  // const unsigned RetEltCount = RetTMaybeVT ? RetTMaybeVT->getNumElements() :
  // 1;

  const llvm::Value *VStrideOpnd = CI.getOperand(SrcOpndIdx + 1);
  const auto *VStrideConstantInt = dyn_cast<llvm::ConstantInt>(VStrideOpnd);
  ensure(VStrideConstantInt, "vertical stride must be a constant int", CI);
  // const unsigned VStride = VStrideConstantInt->getZExtValue();

  const llvm::Value *WidthOpnd = CI.getOperand(SrcOpndIdx + 2);
  const auto WidthConstantInt = dyn_cast<llvm::ConstantInt>(WidthOpnd);

  if (!ensure(WidthConstantInt, "width must be a constant int", CI))
    return;

  const unsigned Width = WidthConstantInt->getZExtValue();

  if (!ensure(Width, "the width must be non-zero.", CI))
    return;

  const unsigned TotalElCount_ExecSize =
      GenXIntrinsic::isRdRegion(IntrinsicId)
          ? RetTMaybeVT ? RetTMaybeVT->getNumElements() : 1
      : SrcOpndMaybeVT ? SrcOpndMaybeVT->getNumElements()
                       : 1;
  ensure(TotalElCount_ExecSize % Width == 0,
         "the width must divide the total size evenly.", CI);

  const llvm::Value *StrideOpnd = CI.getOperand(SrcOpndIdx + 3);
  ensure(isa<llvm::ConstantInt>(StrideOpnd),
         "horizontal stride must be a constant int", CI);

  const llvm::Value *OffsetOpnd = CI.getOperand(SrcOpndIdx + 4);
  const llvm::Type *OffsetOpndT = OffsetOpnd->getType();
  const auto *OffsetOpndMaybeVT =
      dyn_cast<IGCLLVM::FixedVectorType>(OffsetOpndT);

  ensure(OffsetOpndT->getScalarType()->isIntegerTy(16) &&
             (OffsetOpndMaybeVT || isa<llvm::IntegerType>(OffsetOpndT)),
         "offset must be either a fixed vector of i16 or an i16 constant.", CI);

  const unsigned RegionHeight = TotalElCount_ExecSize / Width;
  ensure(!OffsetOpndMaybeVT ||
             (OffsetOpndMaybeVT &&
              OffsetOpndMaybeVT->getNumElements() == RegionHeight),
         "If a vector, then its vector width must be the height of the "
         "region, i.e. the total size of the region divided by the width.",
         CI);

  // The parent width arg is ignored if the offset arg is constant. If the
  // offset arg is variable, then a non-undef parent width is a statement
  // that the value of offset is such that a row of the region does not
  // cross a multiple of parent width boundary. This is used by the backend
  // to determine whether the region can be collapsed into another region.
  const llvm::Value *ParentWidthOpnd = CI.getOperand(SrcOpndIdx + 5);
  if (!isa<llvm::ConstantInt>(OffsetOpnd) &&
      !isa<UndefValue>(ParentWidthOpnd)) {
    const auto ParentWidthConstantInt =
        dyn_cast<llvm::ConstantInt>(ParentWidthOpnd);
    // TODO:spec review: the spec doesn't specify valid value range, but looks
    // like it should.
    ensure(ParentWidthConstantInt && ParentWidthConstantInt->getSExtValue() > 0,
           "parent width when not ignored (offset arg is not a constant) must "
           "be a valid constant integer with the value > 0",
           CI, IsFatal::No);
  }

  if (IntrinsicId == GenXIntrinsic::genx_wrregionf ||
      IntrinsicId == GenXIntrinsic::genx_wrregioni ||
      IntrinsicId == GenXIntrinsic::genx_wrconstregion) {
    const auto *DstSrcOpnd = CI.getOperand(0);
    const auto *DstSrcOpndT = DstSrcOpnd->getType();
    const auto *DstSrcOpndMaybeVT =
        dyn_cast<IGCLLVM::FixedVectorType>(DstSrcOpndT);

    // TODO:spec review: some IRs have arg0 as a scalar with undef value.
    ensure(DstSrcOpndMaybeVT,
           "destination-source (arg0) must be a fixed vector.", CI,
           IsFatal::No);

    // TODO:spec review: some IRs have arg0 as a scalar with undef value.
    ensure(DstSrcOpndMaybeVT,
           "source-destination (arg0) operand must be a fixed vector.", CI,
           IsFatal::No);
    ensure(
        DstSrcOpndT->getScalarType() == SrcOpndT->getScalarType(),
        "The arg1 subvector must have the same element type as the arg0 vector",
        CI);

    if (Stage >= GenXVerifyStage::PostIrAdaptors) {
      if (SrcOpndMaybeVT && DstSrcOpndMaybeVT)
        ensure(SrcOpndMaybeVT->getNumElements() <=
                   DstSrcOpndMaybeVT->getNumElements(),
               "The arg1 subvector must be no larger than arg0 vector", CI);

      // After lowering, the arg1 subvector to write can be a scalar of the
      // same type as an element of arg0, indicating that the region has one
      // element. (Lowering lowers an insertelement to this type of wrregion.)
      ensure(SrcOpndMaybeVT ||
                 (Stage >= GenXVerifyStage::PostGenXLowering && Width == 1),
             "after genx lowering subregion to write may be a scalar if the "
             "number of elements in "
             "subregion is 1.",
             CI);
    }

    ensure(SrcOpndMaybeVT || SrcOpndT->getScalarType(),
           "source (subvector to write) must be of a fixed vector or a scalar "
           "type.",
           CI);

    const llvm::Value *MaskOpnd = CI.getOperand(7);
    const llvm::Type *MaskOpndT = MaskOpnd->getType();
    const auto *MaskOpndMaybeVT =
        dyn_cast<IGCLLVM::FixedVectorType>(MaskOpnd->getType());

    // TODO:spec review: when arg0 non-vector case is resolved recheck
    // SrcOpndMaybeVT necessity.
    if (MaskOpndMaybeVT)
      ensure(
          SrcOpndMaybeVT && MaskOpndMaybeVT->getNumElements() ==
                                SrcOpndMaybeVT->getNumElements(),
          "The arg7 mask can be a vector of booleans, exactly as wide as the "
          "arg1 subvector, such that an element of the subvector is written "
          "into its place in the vector only if the corresponding element of "
          "the "
          "mask is true.",
          CI);
    // TODO:spec review: some IRs have a non-const integer mask operand.
    else
      ensure(MaskOpndT->isIntegerTy(1) && isa<ConstantInt>(MaskOpnd) &&
                 cast<ConstantInt>(MaskOpnd)->getZExtValue() == 1,
             "The arg7 mask can be a single i1 constant with value "
             "1, meaning that the wrregion is unconditional.",
             CI, IsFatal::No);

    if (IntrinsicId == GenXIntrinsic::genx_wrconstregion) {
      ensure(isa<Constant>(SrcOpnd), "subvector to write must be a constant.",
             CI);
      ensure(isa<Constant>(OffsetOpnd), "offset must be a constant.", CI);
    }
  }
}
