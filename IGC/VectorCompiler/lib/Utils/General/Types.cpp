/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"

#include <llvm/Support/Casting.h>
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;

IGCLLVM::FixedVectorType *vc::changeAddrSpace(IGCLLVM::FixedVectorType *OrigTy,
                                              int AddrSpace) {
  IGC_ASSERT_MESSAGE(OrigTy, "wrong argument");
  auto *PointeeTy = OrigTy->getElementType()->getPointerElementType();
  auto EC = OrigTy->getNumElements();
  return IGCLLVM::FixedVectorType::get(
      llvm::PointerType::get(PointeeTy, AddrSpace), EC);
}

Type *vc::changeAddrSpace(Type *OrigTy, int AddrSpace) {
  IGC_ASSERT_MESSAGE(OrigTy, "wrong argument");
  IGC_ASSERT_MESSAGE(
      OrigTy->isPtrOrPtrVectorTy(),
      "wrong argument: pointer or vector of pointers type is expected");
  if (OrigTy->isPointerTy())
    return changeAddrSpace(cast<PointerType>(OrigTy), AddrSpace);
  return changeAddrSpace(cast<IGCLLVM::FixedVectorType>(OrigTy), AddrSpace);
}

int vc::getAddrSpace(Type *PtrOrPtrVec) {
  IGC_ASSERT_MESSAGE(PtrOrPtrVec, "wrong argument");
  IGC_ASSERT_MESSAGE(
      PtrOrPtrVec->isPtrOrPtrVectorTy(),
      "wrong argument: pointer or vector of pointers type is expected");
  if (PtrOrPtrVec->isPointerTy())
    return PtrOrPtrVec->getPointerAddressSpace();
  return cast<VectorType>(PtrOrPtrVec)->getElementType()->getPointerAddressSpace();
}

const Type &vc::fixDegenerateVectorType(const Type &Ty) {
  if (!isa<IGCLLVM::FixedVectorType>(Ty))
    return Ty;
  auto &VecTy = cast<IGCLLVM::FixedVectorType>(Ty);
  if (VecTy.getNumElements() != 1)
    return Ty;
  return *VecTy.getElementType();
}

Type &vc::fixDegenerateVectorType(Type &Ty) {
  return const_cast<Type &>(
      fixDegenerateVectorType(static_cast<const Type &>(Ty)));
}
