/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_INTRINSICS_H
#define IGCLLVM_IR_INTRINSICS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Intrinsics.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
using Intrinsic = llvm::Intrinsic::IndependentIntrinsics;
} // namespace IGCLLVM

#endif
