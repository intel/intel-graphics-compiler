/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenPublic.h"

namespace IGC
{
    void UnifyIROCL(
        OpenCLProgramContext* pContext,
        std::unique_ptr<llvm::Module> BuiltinGenericModule,
        std::unique_ptr<llvm::Module> BuiltinSizeModule);

    void UnifyIRSPIR(
        OpenCLProgramContext* pContext,
        std::unique_ptr<llvm::Module> BuiltinGenericModule,
        std::unique_ptr<llvm::Module> BuiltinSizeModule);
}
