/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class CodeGenContext;

bool needsRegisterAccessBoundsChecks(const CodeGenContext &Ctx);

void initializeRegisterAccessBoundsChecksPass(llvm::PassRegistry &);
llvm::Pass *createRegisterAccessBoundsChecks();
} // namespace IGC
