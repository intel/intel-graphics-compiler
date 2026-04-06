/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

llvm::FunctionPass *createLowerSIMDSizePass();
void initializeLowerSIMDSizePass(llvm::PassRegistry &);
