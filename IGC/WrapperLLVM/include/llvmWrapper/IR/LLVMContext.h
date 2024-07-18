/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_LLVMCONTEXT_H
#define IGCLLVM_IR_LLVMCONTEXT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/LLVMContext.h"

namespace IGCLLVM
{
    // TODO: Clean up obsolete uses at call sites
    class Context : public llvm::LLVMContext
    {
    public:
        void setOpaquePointers(bool Enable) const
        {
#if LLVM_VERSION_MAJOR == 14
            if (Enable)
                enableOpaquePointers();
#elif LLVM_VERSION_MAJOR >= 15
            llvm::LLVMContext::setOpaquePointers(Enable);
#endif // LLVM_VERSION_MAJOR
        }
    };
}

#endif // IGCLLVM_IR_LLVMCONTEXT_H
