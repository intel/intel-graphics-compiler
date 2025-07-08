/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

///////////////////////////////////////////////////////////////////////////////
/// @brief This pass is responsible for eliminating redundant WaveBallot
llvm::FunctionPass *createWaveBallotCSE();
void initializeWaveBallotCSEPass(llvm::PassRegistry &);
