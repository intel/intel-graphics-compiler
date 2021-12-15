/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_TYPES_H
#define VC_UTILS_GENERAL_TYPES_H

#include <llvm/IR/DerivedTypes.h>

#include "llvmWrapper/IR/DerivedTypes.h"

namespace vc {

// Address space indexing follows OCL plus some target specific address spaces.
namespace AddrSpace {
enum Enum {
  Private = 0,
  Global = 1,
  Constant = 2,
  Local = 3,
  Generic = 4,
  // Program addrspace is an addrspace where executable code should be stored.
  Program = 5,
  // Global A32 addrspace is for 32-bit pointers to global memory (global
  // pointers may be 32 or 64 bit, but global A32 pointers are always 32 bit
  // and can coexist with 64 bit pointers).
  GlobalA32 = 6,
};
} // namespace AddrSpace

// Returns potentially new pointer type with the provided \p AddrSpace
// and the original pointee type.
inline llvm::PointerType *changeAddrSpace(llvm::PointerType *OrigTy,
                                          int AddrSpace) {
  return llvm::PointerType::get(OrigTy->getElementType(), AddrSpace);
}

// Changes addrspace inside a vector of pointers type.
IGCLLVM::FixedVectorType *changeAddrSpace(IGCLLVM::FixedVectorType *OrigTy,
                                          int AddrSpace);

// Change addrspace of a pointer or a vector of pointers type.
llvm::Type *changeAddrSpace(llvm::Type *OrigTy, int AddrSpace);

// Get addrspace of a pointer or a vector of pointers type.
int getAddrSpace(llvm::Type *PtrOrPtrVec);

// calculates new return type for cast instructions
// * trunc
// * bitcast
llvm::Type *getNewTypeForCast(llvm::Type *OldOutType, llvm::Type *OldInType,
                              llvm::Type *NewInType);

// If \p Ty is degenerate vector type <1 x ElTy>,
// ElTy is returned, otherwise original type \p Ty is returned.
const llvm::Type &fixDegenerateVectorType(const llvm::Type &Ty);
llvm::Type &fixDegenerateVectorType(llvm::Type &Ty);

// If \p Ty is a fixed vector, it is returned. Otherwise \p Ty is wrapped into
// a degenerate vector: <1 x Ty>.
// To some extend this function is the opposite of vc::fixDegenerateVectorType.
IGCLLVM::FixedVectorType &getVectorType(llvm::Type &Ty);

} // namespace vc

#endif // VC_UTILS_GENERAL_TYPES_H
