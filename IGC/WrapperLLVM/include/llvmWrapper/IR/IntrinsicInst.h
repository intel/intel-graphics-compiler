/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICINST_H
#define IGCLLVM_IR_INTRINSICINST_H

#include <llvm/Config/llvm-config.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/IntrinsicInst.h>

#include "Probe/Assertion.h"

namespace IGCLLVM
{
    inline llvm::Value* getVariableLocation(const llvm::DbgVariableIntrinsic* DbgInst)
    {
        IGC_ASSERT(DbgInst);
#if LLVM_VERSION_MAJOR <= 12
        return DbgInst->getVariableLocation();
#else
        IGC_ASSERT_MESSAGE(DbgInst->getNumVariableLocationOps() == 1,
                           "unsupported number of location ops");
        return DbgInst->getVariableLocationOp(0);
#endif
    }

    inline void setDbgVariableLocationToUndef(llvm::DbgVariableIntrinsic *DbgInst)
    {
        IGC_ASSERT(DbgInst);
#if LLVM_VERSION_MAJOR <= 12
        auto *OP = DbgInst->getVariableLocation();
        IGC_ASSERT_MESSAGE(OP != nullptr, "Empty dbg var not supported");
        auto *Undef = llvm::UndefValue::get(OP->getType());
        DbgInst->setOperand(
            0, llvm::MetadataAsValue::get(DbgInst->getContext(),
                                          llvm::ValueAsMetadata::get(Undef)));
#else
        DbgInst->setUndef();
#endif
    }
}

#endif
