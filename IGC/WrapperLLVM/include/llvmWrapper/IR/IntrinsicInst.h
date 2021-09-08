/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICINST_H
#define IGCLLVM_IR_INTRINSICINST_H

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/IntrinsicInst.h>

#include "Probe/Assertion.h"

namespace IGCLLVM
{
// TODO: remove support for LLVM 7
#if LLVM_VERSION_MAJOR <= 7
    using DbgVariableIntrinsic = llvm::DbgInfoIntrinsic;
#else
    using llvm::DbgVariableIntrinsic;
#endif

    inline llvm::Value* getVariableLocation(const DbgVariableIntrinsic* DbgInst)
    {
        IGC_ASSERT(DbgInst);
#if LLVM_VERSION_MAJOR <= 12
        return DbgInst->getVariableLocation();
#else
        IGC_ASSERT_MESSAGE(getNumVariableLocationOps() == 1,
                           "unsupported number of location ops");
        return DbgInst->getVariableLocationOp(0);
#endif
    }
}

#endif
