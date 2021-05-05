/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICINST_H
#define IGCLLVM_IR_INTRINSICINST_H

#include <llvm/IR/IntrinsicInst.h>


namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 7
    using DbgVariableIntrinsic = llvm::DbgInfoIntrinsic;
#else
    using llvm::DbgVariableIntrinsic;
#endif
}

#endif
