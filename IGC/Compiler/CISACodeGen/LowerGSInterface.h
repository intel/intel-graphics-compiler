/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

// forward declaration of CodeGenContext
namespace IGC
{
    class CodeGenContext;
}

llvm::Pass* createLowerGSInterfacePass(IGC::CodeGenContext* pContext);
