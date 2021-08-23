/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_TYPESIZE_H
#define IGCLLVM_SUPPORT_TYPESIZE_H

#if LLVM_VERSION_MAJOR > 10
#include <llvm/Support/TypeSize.h>
using namespace llvm;
#endif

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 11
inline unsigned getElementCount(unsigned EC) { return EC; }
#elif LLVM_VERSION_MAJOR == 11
inline ElementCount getElementCount(unsigned EC) {
  return ElementCount(EC, false);
}
#else
inline ElementCount getElementCount(unsigned EC) {
  return ElementCount::get(EC, false);
}
#endif
} // namespace IGCLLVM

#endif
