/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_VALUE_H
#define IGCLLVM_IR_VALUE_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Value.h"

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR >= 10
#define stripPointerCastsNoFollowAliases() stripPointerCasts()
#endif
}

#endif
