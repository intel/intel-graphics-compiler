/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_NONE_H
#define IGCLLVM_ADT_NONE_H

#if LLVM_VERSION_MAJOR < 17
#include <llvm/ADT/None.h>
#endif

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR > 15
typedef std::nullopt_t NoneType;
inline constexpr std::nullopt_t None = std::nullopt;
#else
const llvm::NoneType None = llvm::NoneType::None;
#endif
} // namespace IGCLLVM

#endif
