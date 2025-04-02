/*========================== begin_copyright_notice ============================

Copyright (C) 2025-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
llvm::FunctionPass* createIntDivRemIncrementReductionPass();
} // namespace IGC
