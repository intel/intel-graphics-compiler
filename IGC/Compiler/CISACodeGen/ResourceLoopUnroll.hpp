/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

namespace llvm
{
class Pass;
class PassRegistry;
}

namespace IGC
{
////////////////////////////////////////////////////////////////////////
/// @brief This pass is responsible for optimizing resource loop unroll
llvm::Pass* createResourceLoopUnroll();
} // namespace IGC

void initializeResourceLoopUnrollPass(llvm::PassRegistry&);
