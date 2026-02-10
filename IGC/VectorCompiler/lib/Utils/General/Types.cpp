/*========================== begin_copyright_notice ============================

Copyright (C) 2021-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/Types.h"
#include "llvmWrapper/Support/TypeSize.h"

#include "Probe/Assertion.h"

#include <llvm/Support/Casting.h>
#include "llvmWrapper/IR/DerivedTypes.h"

using namespace llvm;

IGCLLVM::FixedVectorType *vc::changeAddrSpace(IGCLLVM::FixedVectorType *OrigTy,
                                              int AddrSpace) {
  IGC_ASSERT_MESSAGE(OrigTy, "wrong argument");
  auto *PtrTy = cast<PointerType>(OrigTy->getElementType());
  auto EC = OrigTy->getNumElements();
  return IGCLLVM::FixedVectorType::get(IGCLLVM::get(PtrTy, AddrSpace), EC);
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

int vc::getAddrSpace(const Type *PtrOrPtrVec) {
  IGC_ASSERT_MESSAGE(PtrOrPtrVec, "wrong argument");
  IGC_ASSERT_MESSAGE(
      PtrOrPtrVec->isPtrOrPtrVectorTy(),
      "wrong argument: pointer or vector of pointers type is expected");
  if (PtrOrPtrVec->isPointerTy())
    return PtrOrPtrVec->getPointerAddressSpace();
  return cast<VectorType>(PtrOrPtrVec)
      ->getElementType()
      ->getPointerAddressSpace();
}

bool vc::isDegenerateVectorType(llvm::Type const &Ty) {
  if (!isa<IGCLLVM::FixedVectorType>(Ty))
    return false;
  auto &VecTy = cast<IGCLLVM::FixedVectorType>(Ty);
  return VecTy.getNumElements() == 1;
}

const Type &vc::fixDegenerateVectorType(const Type &Ty) {
  if (!isDegenerateVectorType(Ty))
    return Ty;
  return *(cast<IGCLLVM::FixedVectorType>(Ty).getElementType());
}

Type &vc::fixDegenerateVectorType(Type &Ty) {
  return const_cast<Type &>(
      fixDegenerateVectorType(static_cast<const Type &>(Ty)));
}

// calculates new return type for cast instructions
// * trunc
// * bitcast
// Expect that scalar type of instruction not changed and previous
// combination of OldOutType & OldInType is valid
Type *vc::getNewTypeForCast(Type *OldOutType, Type *OldInType,
                            Type *NewInType) {
  IGC_ASSERT_MESSAGE(OldOutType && NewInType && OldInType,
                     "Error: nullptr input");

  auto NewInVecType = dyn_cast<IGCLLVM::FixedVectorType>(NewInType);
  auto OldOutVecType = dyn_cast<IGCLLVM::FixedVectorType>(OldOutType);
  auto OldInVecType = dyn_cast<IGCLLVM::FixedVectorType>(OldInType);

  bool NewInIsPtrOrVecPtr = NewInType->isPtrOrPtrVectorTy();
  [[maybe_unused]] bool OldOutIsPtrOrVecPtr = OldOutType->isPtrOrPtrVectorTy();
  [[maybe_unused]] bool OldInIsPtrOrVecPtr = OldInType->isPtrOrPtrVectorTy();

  // only  pointer to pointer
  IGC_ASSERT(NewInIsPtrOrVecPtr == OldOutIsPtrOrVecPtr &&
             NewInIsPtrOrVecPtr == OldInIsPtrOrVecPtr);

  // <2 x char> -> int : < 4 x char> -> ? forbidden
  IGC_ASSERT((bool)OldOutVecType == (bool)OldInVecType &&
             (bool)OldOutVecType == (bool)NewInVecType);

  Type *NewOutType = OldOutType;
  if (OldOutVecType) {
    // <4 x char> -> <2 x int> : <8 x char> -> <4 x int>
    // <4 x char> -> <2 x int> : <2 x char> -> <1 x int>
    auto NewInEC = NewInVecType->getNumElements();
    auto OldOutEC = OldOutVecType->getNumElements();
    auto OldInEC = OldInVecType->getNumElements();
    auto NewOutEC = OldOutEC * NewInEC / OldInEC;
    // <4 x char> -> <2 x int> : <5 x char> -> ? forbidden
    IGC_ASSERT_MESSAGE((OldOutEC * NewInEC) % OldInEC == 0,
                       "Error: wrong combination of input/output");
    // element count changed, scalar type as previous
    NewOutType = IGCLLVM::FixedVectorType::get(OldOutVecType->getElementType(),
                                               NewOutEC);
  }

  IGC_ASSERT(NewOutType);

  if (NewInIsPtrOrVecPtr) {
    // address space from new
    // element count calculated as for vector
    // element type expect address space similar
    auto AddressSpace = getAddrSpace(NewInType);
    return changeAddrSpace(NewOutType, AddressSpace);
  }
  // <4 x char> -> <2 x half> : < 2 x int> - ? forbiddeb
  IGC_ASSERT_MESSAGE(OldInType->getScalarType() == NewInType->getScalarType(),
                     "Error: unexpected type change");
  return NewOutType;
}

IGCLLVM::FixedVectorType &vc::getVectorType(Type &Ty) {
  if (isa<IGCLLVM::FixedVectorType>(Ty))
    return cast<IGCLLVM::FixedVectorType>(Ty);
  return *IGCLLVM::FixedVectorType::get(&Ty, 1);
}

Type *vc::setScalarType(Type &OrigTy, Type &ScalarTy) {
  IGC_ASSERT_MESSAGE(OrigTy.isFPOrFPVectorTy() || OrigTy.isIntOrIntVectorTy() ||
                         OrigTy.isPtrOrPtrVectorTy(),
                     "wrong argument: OrigType must be an int, float, pointer "
                     "or vector of those");
  IGC_ASSERT_MESSAGE(
      ScalarTy.isFloatingPointTy() || ScalarTy.isIntegerTy() ||
          ScalarTy.isPointerTy(),
      "wrong argument: ScalarTy must be an int, float or pointer");
  if (!isa<IGCLLVM::FixedVectorType>(OrigTy))
    return &ScalarTy;
  return IGCLLVM::FixedVectorType::get(
      &ScalarTy, cast<IGCLLVM::FixedVectorType>(OrigTy).getNumElements());
}

unsigned vc::getNumElements(Type &Ty) {
  if (isa<IGCLLVM::FixedVectorType>(Ty))
    return cast<IGCLLVM::FixedVectorType>(Ty).getNumElements();
  if (isa<ArrayType>(Ty))
    return cast<ArrayType>(Ty).getNumElements();
  if (isa<StructType>(Ty))
    return cast<StructType>(Ty).getNumElements();
  return 1;
}

bool vc::isFunctionPointerType(Type *Ty) {
  auto *PtrTy = llvm::dyn_cast<PointerType>(Ty);
  if (!PtrTy)
    return false;
  return PtrTy->getAddressSpace() == AddrSpace::CodeSectionINTEL;
}
