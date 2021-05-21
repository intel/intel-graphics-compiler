/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_STLEXTRAS_H
#define IGCLLVM_ADT_STLEXTRAS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/ADT/STLExtras.h"
#if LLVM_VERSION_MAJOR > 9
#include <memory>
#endif

namespace IGCLLVM
{
#if LLVM_VERSION_MAJOR <= 9
    using llvm::make_unique;
#else
    using std::make_unique;
#endif
}

#endif
