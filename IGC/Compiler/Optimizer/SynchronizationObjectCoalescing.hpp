/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

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
/// @brief This pass is responsible for reducing the number of synchronization
/// instruction. Moreover, the necessary memory fences can decrease their
/// scope of influence.
llvm::Pass* createSynchronizationObjectCoalescing();
} // namespace IGC

void initializeSynchronizationObjectCoalescingPass(llvm::PassRegistry&);
