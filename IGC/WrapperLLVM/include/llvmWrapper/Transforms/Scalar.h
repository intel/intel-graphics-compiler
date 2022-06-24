/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_H
#define IGCLLVM_TRANSFORMS_SCALAR_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Scalar.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR < 7
#error Not supported llvm version.
#elif LLVM_VERSION_MAJOR == 7
    using llvm::createLoopUnrollPass;
#elif LLVM_VERSION_MAJOR == 8
    inline static llvm::Pass* createLoopUnrollPass(
        int OptLevel = 2, int Threshold = -1, int Count = -1,
        int AllowPartial = -1, int Runtime = -1,
        int UpperBound = -1, int AllowPeeling = -1)
    {
        return llvm::createLoopUnrollPass(OptLevel, false, Threshold, Count, AllowPartial, Runtime, UpperBound, AllowPeeling);
    }
#elif LLVM_VERSION_MAJOR >= 9 && LLVM_VERSION_MAJOR <= 14
    inline static llvm::Pass * createLoopUnrollPass(
        int OptLevel = 2, int Threshold = -1, int Count = -1,
        int AllowPartial = -1, int Runtime = -1,
        int UpperBound = -1, int AllowPeeling = -1)
    {
        return llvm::createLoopUnrollPass(OptLevel, false, false, Threshold, Count, AllowPartial, Runtime, UpperBound, AllowPeeling);
    }
#else
    //DO NOT assume same function signature for all incoming llvm versions! Double check to upgrade!
#error Not supported llvm version.
#endif
}
#endif
