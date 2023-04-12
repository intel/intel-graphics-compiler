/*========================== begin_copyright_notice ============================

Copyright (C) 2022-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

////////////////////////////////////////////////////////////////////////////
// This pass stops propagation of poison values returned in case of integer
// division by zero. LLVM 10+ freeze instruction is used for that purpose.

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
    llvm::FunctionPass* createFreezeIntDivPass();
    void initializeFreezeIntDivPass(llvm::PassRegistry&);
}
