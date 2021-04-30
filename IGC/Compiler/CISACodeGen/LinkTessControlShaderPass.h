/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef __LINK_TESS_CONTROL_SHADER__
#define __LINK_TESS_CONTROL_SHADER__

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/InitializePasses.h"
void initializeLinkTessControlShaderPass(llvm::PassRegistry&);

namespace IGC
{
    llvm::Pass* createLinkTessControlShader();
}

#endif
