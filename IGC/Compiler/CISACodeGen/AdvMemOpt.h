/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_ADVMEMOPT_H_
#define _CISA_ADVMEMOPT_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    void initializeAdvMemOptPass(llvm::PassRegistry&);
    llvm::FunctionPass* createAdvMemOptPass();
} // End namespace IGC

#endif // _CISA_ADVMEMOPT_H_
