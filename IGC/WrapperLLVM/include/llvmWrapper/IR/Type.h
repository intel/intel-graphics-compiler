/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_TYPE_H
#define IGCLLVM_IR_TYPE_H

#include "llvm/IR/Type.h"

namespace IGCLLVM {
/// Only use this method in code that is not reachable with opaque pointers,
/// or part of deprecated methods that will be removed as part of the opaque
/// pointers transition. (For LLVM 14+)
// Normally, we'd want the wrapper function name to match that in the
// latest relevant LLVM revision to simplify migration. In this instance,
// a shorthand is chosen primarily for formatting reasons.
inline llvm::Type *getNonOpaquePtrEltTy(const llvm::Type *PtrTy) {
  return PtrTy->getNonOpaquePointerElementType();
}

inline bool isOpaquePointerTy(const llvm::Type *PtrTy) {
  return PtrTy->isOpaquePointerTy();
}

inline bool isBFloatTy(llvm::Type *type) {
  return type->getTypeID() == llvm::Type::TypeID::BFloatTyID;
}

inline bool isTargetExtTy(const llvm::Type *PtrTy) {
#if LLVM_VERSION_MAJOR < 16
  // not supported below LLVM 16
  // to be removed once we switch to 16
  return false;
#else
  return PtrTy->isTargetExtTy();
#endif
}

inline llvm::StringRef getTargetExtName(const llvm::Type *PtrTy) {
#if LLVM_VERSION_MAJOR < 16
  // not supported below LLVM 16
  // to be removed once we switch to 16
  return llvm::StringRef();
#else
  return PtrTy->getTargetExtName();
#endif
}
} // namespace IGCLLVM

#endif // IGCLLVM_IR_TYPE_H