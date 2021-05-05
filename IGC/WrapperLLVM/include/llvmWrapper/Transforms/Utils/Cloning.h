/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_CLONING_H
#define IGCLLVM_TRANSFORMS_UTILS_CLONING_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/Cloning.h"

namespace IGCLLVM
{
    inline bool InlineFunction(llvm::CallInst* CB, llvm::InlineFunctionInfo& IFI,
        llvm::AAResults* CalleeAAR = nullptr,
        bool InsertLifetime = true,
        llvm::Function* ForwardVarArgsTo = nullptr)
    {
        return llvm::InlineFunction(
#if LLVM_VERSION_MAJOR >= 11
            *
#endif
            CB, IFI, CalleeAAR, InsertLifetime, ForwardVarArgsTo)
#if LLVM_VERSION_MAJOR >= 11
            .isSuccess()
#endif
            ;
    }
}

#endif
