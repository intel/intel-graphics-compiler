/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CALLSITE_H
#define IGCLLVM_IR_CALLSITE_H

#include "llvm/Config/llvm-config.h"
#if LLVM_VERSION_MAJOR <= 10
#include "llvm/IR/CallSite.h"
#else
#include "llvm/IR/InstrTypes.h"
#endif

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 10
    using llvm::CallSite;
    using CallSiteRef = IGCLLVM::CallSite;
#else
    using CallSite = llvm::CallBase;
    using CallSiteRef = IGCLLVM::CallSite&;
#endif
}

#endif

