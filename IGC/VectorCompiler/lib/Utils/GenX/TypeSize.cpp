/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/Types.h"
#include "vc/Utils/GenX/TypeSize.h"
#include "llvmWrapper/IR/Type.h"
#include "llvm/IR/DataLayout.h"

#include <string>

// The data layout specifies the function pointer size only for targets where
// function pointers are not 32-bit.
static bool hasFunctionPointerSpec(const llvm::DataLayout &DL) {
  static const std::string Spec =
      "-p" + std::to_string(vc::AddrSpace::CodeSectionINTEL) + ":";
  return DL.getStringRepresentation().find(Spec) != std::string::npos;
}

vc::TypeSizeWrapper vc::getTypeSize(llvm::Type *Ty,
                                    const llvm::DataLayout *DL) {
  IGC_ASSERT(Ty && Ty->isSized());
  // FIXME: it's better to use DataLayout to get function pointers type size, so
  // we should remove it when such pointers will be in separate address space.
  // Size of function pointers is 32 bit unless DataLayout specifies it.
  if (vc::isFunctionPointerType(Ty->getScalarType()) &&
      !(DL && hasFunctionPointerSpec(*DL))) {
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
