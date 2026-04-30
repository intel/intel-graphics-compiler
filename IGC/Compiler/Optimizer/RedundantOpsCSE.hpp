/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

///////////////////////////////////////////////////////////////////////////////
/// @brief This pass is responsible for eliminating redundant ResourceDescriptorHeap
llvm::FunctionPass *createRedundantOpsCSEPass();
void initializeRedundantOpsCSEPassPass(llvm::PassRegistry &);
