/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ADT_OPTIONAL_H
#define IGCLLVM_ADT_OPTIONAL_H

#include <llvm/ADT/Optional.h>

namespace IGCLLVM {
template <typename T> class Optional : public llvm::Optional<T> {
public:
  using BaseT = llvm::Optional<T>;
  constexpr Optional(const BaseT &O) : BaseT(O) {}
  constexpr Optional(BaseT &&O) : BaseT(std::move(O)) {}

/* ---------------------|
| Deprecated in LLVM 15 |
| -------------------- */
#if LLVM_VERSION_MAJOR < 15
  template <typename U> constexpr T value_or(U &&alt) const & {
    return this->getValueOr(std::forward<U>(alt));
  }

  template <typename U> T value_or(U &&alt) && {
    return this->getValueOr(std::forward<U>(alt));
  }
#endif

  // TODO: Once relevant, add similar wrappers per LLVM 16 deprecations.
  // Example:
  // T &value() &noexcept {
  //   return getValue();
  // }
};

template <typename T>
Optional<T> wrapOptional(const llvm::Optional<T> &O) {
  return { O };
}
} // namespace IGCLLVM

#endif // IGCLLVM_ADT_OPTIONAL_H
