/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICS_H
#define IGCLLVM_IR_INTRINSICS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Intrinsics.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR < 10
    using Intrinsic = llvm::Intrinsic::ID;
#else
    using Intrinsic = llvm::Intrinsic::IndependentIntrinsics;
#endif
}

#endif
