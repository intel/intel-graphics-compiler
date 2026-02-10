/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_STLEXTRAS_H
#define IGCLLVM_ADT_STLEXTRAS_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/STLExtras.h"
#include "IGC/common/LLVMWarningsPop.hpp"
#include <memory>

namespace IGCLLVM {
using std::make_unique;
}

#endif
