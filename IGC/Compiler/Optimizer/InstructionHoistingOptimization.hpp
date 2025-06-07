/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm {
class Pass;
class PassRegistry;
} // namespace llvm

namespace IGC {
////////////////////////////////////////////////////////////////////////
/// @brief This pass is responsible for hoisting latency instructions
llvm::Pass *createInstructionHoistingOptimization();
} // namespace IGC

void initializeInstructionHoistingOptimizationPass(llvm::PassRegistry &);
