/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/// Splits the back-edge of every loop whose header and latch both exit, so that LoopRotate can rotate the loop.
llvm::FunctionPass *createSplitExitingLatchBackedgePass();
} // namespace IGC
