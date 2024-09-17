/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_OPTIONAL_H
#define IGCLLVM_ADT_OPTIONAL_H

#include <llvm/ADT/Optional.h>
#include <optional>

namespace IGCLLVM {
template <typename T>
    static std::optional<T> makeOptional(const llvm::Optional<T>& O) {
#if LLVM_VERSION_MAJOR < 16
        return O.hasValue() ? std::optional<T>(O.getValue()) : std::optional<T>(std::nullopt);
#else
        return O.has_value() ? std::optional<T>(O.value()) : std::optional<T>(std::nullopt);
#endif
    }
} // namespace IGCLLVM

#endif // IGCLLVM_ADT_OPTIONAL_H
