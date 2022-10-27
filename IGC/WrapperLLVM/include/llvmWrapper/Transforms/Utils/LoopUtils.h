/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_LOOPUTILS_H
#define IGCLLVM_TRANSFORMS_UTILS_LOOPUTILS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 8
    using llvm::InsertPreheaderForLoop;
#elif LLVM_VERSION_MAJOR > 8
    inline llvm::BasicBlock *InsertPreheaderForLoop(llvm::Loop *L, llvm::DominatorTree *DT, llvm::LoopInfo *LI,
                                   bool PreserveLCSSA)
    {
        return llvm::InsertPreheaderForLoop(L, DT, LI, nullptr, PreserveLCSSA);
    }
#endif

    inline bool isInnermost(llvm::Loop *L) {
#if LLVM_VERSION_MAJOR >= 12
        return L->isInnermost();
#else
        return L->empty();
#endif
    }
}

#endif
