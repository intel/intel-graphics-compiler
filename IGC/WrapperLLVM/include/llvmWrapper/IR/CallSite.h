/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CALLSITE_H
#define IGCLLVM_IR_CALLSITE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstrTypes.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
using CallSite = llvm::CallBase;
using CallSiteRef = IGCLLVM::CallSite &;
} // namespace IGCLLVM

#endif
