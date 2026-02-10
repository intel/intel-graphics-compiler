/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_OPTIONAL_H
#define IGCLLVM_ADT_OPTIONAL_H

#if LLVM_VERSION_MAJOR < 17 || defined(IGC_LLVM_TRUNK_REVISION)
#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/ADT/Optional.h>
#include "IGC/common/LLVMWarningsPop.hpp"
#endif
#include <optional>

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 16
template <class T> using optional = llvm::Optional<T>;

template <class T> T getValue(optional<T> data) { return data.getValue(); }
#else
template <class T> using optional = std::optional<T>;

template <class T> T getValue(optional<T> data) { return data.value(); }
#endif

template <typename T> static std::optional<T> makeOptional(const IGCLLVM::optional<T> &O) {
#if LLVM_VERSION_MAJOR < 16
  return O.hasValue() ? std::optional<T>(O.getValue()) : std::optional<T>(std::nullopt);
#else
  return O;
#endif
}

template <typename T> optional<T> makeLLVMOptional(const std::optional<T> &O) {
#if LLVM_VERSION_MAJOR < 16
  return O.has_value() ? llvm::Optional<T>(O.value()) : llvm::Optional<T>();
#else
  return O;
#endif
}
} // namespace IGCLLVM

#endif // IGCLLVM_ADT_OPTIONAL_H
