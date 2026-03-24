/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_ENDIAN_H
#define IGCLLVM_SUPPORT_ENDIAN_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Support/Endian.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
namespace endianness {

#if LLVM_VERSION_MAJOR >= 22
inline constexpr auto little = llvm::endianness::little;
#else
inline constexpr auto little = llvm::support::little;
#endif

} // namespace endianness
} // namespace IGCLLVM

#endif
