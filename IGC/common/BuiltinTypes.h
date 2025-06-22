/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/DerivedTypes.h"
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

bool isTargetExtTy(llvm::Type *Ty);
bool isImageBuiltinType(llvm::Type *BuiltinTy);

#if LLVM_VERSION_MAJOR >= 16
void retypeTargetExtTyArgs(llvm::Module *M);
#endif

} // namespace IGC
