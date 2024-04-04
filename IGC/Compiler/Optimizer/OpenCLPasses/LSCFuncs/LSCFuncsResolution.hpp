/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/Types.hpp"

namespace llvm {
    class FunctionPass;
}

namespace IGC
{
    // LSC 2D block address payload field names for updating only
    // (block width/height/numBlock are not updated).
    enum LSC2DBlockField {
        BASE=1, WIDTH=2, HEIGHT=3, PITCH=4, BLOCKX=5, BLOCKY=6
    };

    llvm::FunctionPass* createLSCFuncsResolutionPass();
} // namespace IGC

