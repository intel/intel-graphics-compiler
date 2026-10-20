/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// FoldProvableFCmp.hpp - Fold fcmp insts that FPRangeAnalysis can simplify
//
//===----------------------------------------------------------------------===//

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
llvm::FunctionPass *createFoldProvableFCmpPass();
} // namespace IGC
