/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CALLSITE_H
#define IGCLLVM_IR_CALLSITE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/InstrTypes.h"

namespace IGCLLVM {
using CallSite = llvm::CallBase;
using CallSiteRef = IGCLLVM::CallSite &;
} // namespace IGCLLVM

#endif
