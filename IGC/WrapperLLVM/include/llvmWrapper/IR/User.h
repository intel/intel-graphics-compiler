/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_USER_H
#define IGCLLVM_IR_USER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/User.h"

namespace IGCLLVM
{
    inline bool isDroppable(llvm::User* U)
    {
#if LLVM_VERSION_MAJOR >= 11
        return U->isDroppable();
#else // For earlier versions, simply copy LLVM 11's implementation
        if (const auto* Intr = llvm::dyn_cast<llvm::IntrinsicInst>(U))
            return Intr->getIntrinsicID() == llvm::Intrinsic::assume;
        return false;
#endif
    }
}

#endif // IGCLLVM_IR_USER_H