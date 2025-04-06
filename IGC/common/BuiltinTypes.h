/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DerivedTypes.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

bool isImageBuiltinType(llvm::Type *builtinTy);

} // namespace IGC
