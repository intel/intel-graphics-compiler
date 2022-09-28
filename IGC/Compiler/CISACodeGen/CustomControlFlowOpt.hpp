/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/IGCPassSupport.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include <Probe/Assertion.h>

namespace IGC {
llvm::FunctionPass *createCustomControlFlowOptPass();
void initializeCustomControlFlowOptPass(llvm::PassRegistry &);
} // namespace IGC
