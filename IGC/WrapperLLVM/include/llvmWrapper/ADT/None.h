/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_NONE_H
#define IGCLLVM_ADT_NONE_H

#include "IGC/common/LLVMWarningsPush.hpp"
#if LLVM_VERSION_MAJOR < 17 || defined(IGC_LLVM_TRUNK_REVISION)
#include <llvm/ADT/None.h>
#endif
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR > 15
typedef std::nullopt_t NoneType;
inline constexpr std::nullopt_t None = std::nullopt;
#else
const llvm::NoneType None = llvm::NoneType::None;
#endif
} // namespace IGCLLVM

#endif
