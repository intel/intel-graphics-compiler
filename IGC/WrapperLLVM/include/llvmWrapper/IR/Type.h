/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_TYPE_H
#define IGCLLVM_IR_TYPE_H

#include "llvm/IR/Type.h"

namespace IGCLLVM {
    // Normally, we'd want the wrapper function name to match that in the
    // latest relevant LLVM revision to simplify migration. In this instance,
    // a shorthand is chosen primarily for formatting reasons.
    inline llvm::Type *getNonOpaquePtrEltTy(const llvm::Type *PtrTy) {
#if LLVM_VERSION_MAJOR < 14
        return PtrTy->getPointerElementType();
#else
        return PtrTy->getNonOpaquePointerElementType();
#endif
    }

    inline bool isBFloatTy(llvm::Type* type) {
#if LLVM_VERSION_MAJOR < 14
        return false;
#else
        return type->getTypeID() == llvm::Type::TypeID::BFloatTyID;
#endif
    }
}

#endif // IGCLLVM_IR_TYPE_H