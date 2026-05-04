/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

///////////////////////////////////////////////////////////////////////////////
/// @brief This pass is responsible for eliminating redundant ResourceDescriptorHeap
llvm::FunctionPass *createRedundantOpsCSEPass();
llvm::FunctionPass *createRedundantOpsCSEPass(bool enableCrossBB);
void initializeRedundantOpsCSEPassPass(llvm::PassRegistry &);
