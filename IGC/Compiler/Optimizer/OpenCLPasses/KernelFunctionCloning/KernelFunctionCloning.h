/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPENCL_KERNELFUNCTIONCLONING_H_
#define _OPENCL_KERNELFUNCTIONCLONING_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

    void initializeKernelFunctionCloningPass(llvm::PassRegistry&);
    llvm::ModulePass* createKernelFunctionCloningPass();

} // End IGC namespace

#endif // _OPENCL_KERNELFUNCTIONCLONING_H_

