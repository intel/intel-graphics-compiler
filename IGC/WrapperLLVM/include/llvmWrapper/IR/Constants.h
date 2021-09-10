/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_CONSTANTS_H
#define IGCLLVM_IR_CONSTANTS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Constants.h"

#if LLVM_VERSION_MAJOR > 12
#include "llvm/Support/TypeSize.h"
#endif

namespace IGCLLVM
{
  inline
#if LLVM_VERSION_MAJOR <= 12
  unsigned
#else
  llvm::ElementCount
#endif
  getElementCount(const llvm::ConstantAggregateZero &C) {
#if LLVM_VERSION_MAJOR <= 12
    return C.getNumElements();
#else
    return C.getElementCount();
#endif
  }
}

#endif
