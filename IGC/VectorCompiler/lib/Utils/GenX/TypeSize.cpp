/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/TypeSize.h"

vc::TypeSizeWrapper vc::getTypeSize(llvm::Type *Ty,
                                    const llvm::DataLayout *DL) {
  IGC_ASSERT(Ty && Ty->isSized());
  // FIXME: it's better to use DataLayout to get function pointers type size, so
  // we should remove it when such pointers will be in separate address space.
  // Size of function pointers is always 32 bit.
  if (auto *PT = llvm::dyn_cast<llvm::PointerType>(Ty->getScalarType());
      PT && PT->getPointerElementType()->isFunctionTy()) {
    // FIXME: wrong condition.
    auto NumElements =
        llvm::isa<IGCLLVM::FixedVectorType>(Ty)
            ? llvm::cast<IGCLLVM::FixedVectorType>(Ty)->getNumElements()
            : 1;
    IGC_ASSERT_MESSAGE(vc::DWordBits * NumElements / vc::DWordBits ==
                           NumElements,
                       "input vector type is too large to operate on");
    auto SizeInBits = vc::DWordBits * NumElements;
    return vc::TypeSizeWrapper::FixedDLSize(SizeInBits);
  }

  auto Ts = DL ? DL->getTypeSizeInBits(Ty) : Ty->getPrimitiveSizeInBits();
  IGC_ASSERT_MESSAGE(Ts != vc::TypeSizeWrapper::InvalidDLSize(),
                     "Consider using DataLayout for retrieving this type size");
  return Ts;
}
