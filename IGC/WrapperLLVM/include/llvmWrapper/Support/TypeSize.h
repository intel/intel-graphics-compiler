/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_TYPESIZE_H
#define IGCLLVM_SUPPORT_TYPESIZE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/Support/TypeSize.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::ElementCount getElementCount(unsigned EC) { return llvm::ElementCount::get(EC, false); }
using TypeSize = llvm::TypeSize;
inline llvm::TypeSize getTypeSize(unsigned TS) { return llvm::TypeSize::get(TS, false); }
} // namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_TYPESIZE_H
