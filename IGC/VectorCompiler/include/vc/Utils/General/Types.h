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
// Program addrspace is an addrspace where executable code should be stored.
namespace AddrSpace {
enum Enum {
  Private = 0,
  Global = 1,
  Constant = 2,
  Local = 3,
  Generic = 4,
  Program = 5,
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

} // namespace vc

#endif // VC_UTILS_GENERAL_TYPES_H
