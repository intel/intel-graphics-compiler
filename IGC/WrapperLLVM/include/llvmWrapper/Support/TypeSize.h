/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_TYPESIZE_H
#define IGCLLVM_SUPPORT_TYPESIZE_H

#if LLVM_VERSION_MAJOR > 10
#include <llvm/Support/TypeSize.h>
#endif

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 11
inline unsigned getElementCount(unsigned EC) { return EC; }
#elif LLVM_VERSION_MAJOR == 11
inline llvm::ElementCount getElementCount(unsigned EC) {
  return llvm::ElementCount(EC, false);
}
#else
inline llvm::ElementCount getElementCount(unsigned EC) {
  return llvm::ElementCount::get(EC, false);
}
#endif
#if LLVM_VERSION_MAJOR <= 11
using TypeSize = unsigned;
inline IGCLLVM::TypeSize getTypeSize(unsigned TS) { return TS; }
#else
using TypeSize = llvm::TypeSize;
inline llvm::TypeSize getTypeSize(unsigned TS) {
  return llvm::TypeSize::get(TS, false);
}
#endif
} // namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_TYPESIZE_H
