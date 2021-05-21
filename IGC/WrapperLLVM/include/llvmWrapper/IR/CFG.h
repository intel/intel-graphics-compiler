/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CFG_H
#define IGCLLVM_IR_CFG_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/CFG.h"

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 11
    using llvm::const_succ_iterator;
#else
    using const_succ_iterator = llvm::succ_const_iterator;
#endif
}

#endif
