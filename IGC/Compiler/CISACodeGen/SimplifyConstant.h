/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

void initializeSimplifyConstantPass(llvm::PassRegistry &);
llvm::ModulePass *createSimplifyConstantPass();

void initializePromoteConstantPass(llvm::PassRegistry &);
llvm::FunctionPass *createPromoteConstantPass();

} // namespace IGC
