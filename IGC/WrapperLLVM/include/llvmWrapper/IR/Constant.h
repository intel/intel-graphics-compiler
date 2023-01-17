/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANT_H
#define IGCLLVM_IR_CONSTANT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/DerivedTypes.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR == 8
    using Constant = llvm::Constant*;
#else
    using Constant = llvm::FunctionCallee;
#endif
}

#endif
