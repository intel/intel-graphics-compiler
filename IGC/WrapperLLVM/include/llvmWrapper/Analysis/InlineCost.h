/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_INLINECOST_H
#define IGCLLVM_ANALYSIS_INLINECOST_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/InlineCost.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 7
    using llvm::InlineCost;
#elif LLVM_VERSION_MAJOR >= 8
    class InlineCost : public llvm::InlineCost
    {
    public:
        static inline llvm::InlineCost getAlways()
        {
            return llvm::InlineCost::getAlways("");
        }
        static inline llvm::InlineCost getNever()
        {
            return llvm::InlineCost::getNever("");
        }
    };
#endif
}

#endif
