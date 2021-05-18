/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXVisa.h"

#include "GenX.h"

#include <llvm/Support/MathExtras.h>

#include "Probe/Assertion.h"

using namespace llvm;

bool visa::Variable::General::isLegal(int ElemSize, int NumElems) {
  IGC_ASSERT_MESSAGE(ElemSize > 0 && NumElems > 0,
                     "wrong arguments: must be positive");
  IGC_ASSERT_MESSAGE(isPowerOf2_32(ElemSize) && ElemSize <= genx::QWordBytes,
                     "wrong arguments: element size must be a power of 2, "
                     "less or equal to QWord bytes");
  // vISA spec: Num elements valid range is [1, 4096], and the variable size
  //            (num_elements * sizeof(type)) must be less than 4K bytes.
  // Probably "less or equal 4K" was meant.
  return NumElems <= visa::Variable::General::MaxNumElements &&
         ElemSize * NumElems <= visa::Variable::General::MaxSizeInBytes;
}

bool visa::Variable::Predicate::isLegal(int NumElems) {
  IGC_ASSERT_MESSAGE(NumElems > 0, "wrong arguments: must be positive");
  return NumElems <= visa::Variable::Predicate::MaxNumElements &&
         isPowerOf2_32(NumElems);
}

bool visa::Variable::General::isLegal(IGCLLVM::FixedVectorType &Ty,
                                      const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(!IGCLLVM::isScalable(Ty),
                     "wrong arguments: scalable vectors aren't supported");
  IGC_ASSERT_MESSAGE(DL.getTypeSizeInBits(Ty.getElementType()) >=
                         genx::ByteBits,
                     "wrong arguments: general variables don't hold bools");
  return visa::Variable::General::isLegal(
      DL.getTypeSizeInBits(Ty.getElementType()) / genx::ByteBits,
      Ty.getNumElements());
}

bool visa::Variable::Predicate::isLegal(IGCLLVM::FixedVectorType &Ty) {
  IGC_ASSERT_MESSAGE(!IGCLLVM::isScalable(Ty),
                     "wrong arguments: scalable vectors aren't supported");
  IGC_ASSERT_MESSAGE(Ty.isIntOrIntVectorTy(1),
                     "wrong arguments: vector of bool is expected");
  return visa::Variable::Predicate::isLegal(Ty.getNumElements());
}

bool visa::Variable::isLegal(IGCLLVM::FixedVectorType &Ty,
                             const DataLayout &DL) {
  IGC_ASSERT_MESSAGE(!IGCLLVM::isScalable(Ty),
                     "wrong arguments: scalable vectors aren't supported");
  if (Ty.isIntOrIntVectorTy(1))
    return visa::Variable::Predicate::isLegal(Ty);
  return visa::Variable::General::isLegal(Ty, DL);
}
