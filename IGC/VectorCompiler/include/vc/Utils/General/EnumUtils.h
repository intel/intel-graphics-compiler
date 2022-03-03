/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

// Helpers for generic enums, which underlying vals must be continuous,
// starting with 0. MaxValue here must not be real member of enum, but rather
// "position, following the last element"

#ifndef VC_UTILS_GENERAL_ENUMUTILS_H
#define VC_UTILS_GENERAL_ENUMUTILS_H

#include "Probe/Assertion.h"

#include <iterator>
#include <type_traits>

namespace vc {

template <typename Enum, Enum LastValue> class EnumIterator final {
private:
  // Just an enum
  static_assert(
      std::is_same_v<std::remove_cv_t<std::remove_reference_t<Enum>>, Enum>);
  static_assert(std::is_integral_v<std::underlying_type_t<Enum>>);

  Enum CurVal;

public:
  using difference_type = std::ptrdiff_t;
  using value_type = Enum;
  using pointer = void;
  using reference = Enum const &;
  using iterator_category = std::bidirectional_iterator_tag;

public:
  constexpr EnumIterator()
      : CurVal(static_cast<Enum>(
            static_cast<std::underlying_type_t<Enum>>(LastValue) + 1)) {}
  constexpr EnumIterator(Enum Value) : CurVal(Value) {
    IGC_ASSERT(checkEnumValue(CurVal));
  }

  constexpr reference operator*() const {
    IGC_ASSERT(checkEnumValue(CurVal));
    return CurVal;
  }

  constexpr EnumIterator &operator++() {
    IGC_ASSERT(checkEnumValue(CurVal));
    CurVal = static_cast<Enum>(
        static_cast<std::underlying_type_t<Enum>>(CurVal) + 1);
    return *this;
  }

  constexpr EnumIterator operator++(int) {
    IGC_ASSERT(checkEnumValue(CurVal));
    auto RetVal = CurVal;
    operator++();
    return RetVal;
  }

  constexpr EnumIterator &operator--() {
    CurVal = static_cast<Enum>(
        static_cast<std::underlying_type_t<Enum>>(CurVal) - 1);
    IGC_ASSERT(checkEnumValue(CurVal));
    return *this;
  }

  constexpr EnumIterator operator--(int) {
    auto RetVal = CurVal;
    operator--();
    IGC_ASSERT(checkEnumValue(CurVal));
    return RetVal;
  }

  constexpr friend bool operator==(const EnumIterator &lhs,
                                   const EnumIterator &rhs) {
    return lhs.CurVal == rhs.CurVal;
  }
  constexpr friend bool operator!=(const EnumIterator &lhs,
                                   const EnumIterator &rhs) {
    return !(lhs == rhs);
  }

private:
  constexpr static bool checkEnumValue(Enum Value) {
    auto IntLastVal = static_cast<std::underlying_type_t<Enum>>(LastValue);
    auto IntVal = static_cast<std::underlying_type_t<Enum>>(Value);
    if constexpr (std::is_signed_v<std::underlying_type_t<Enum>>) {
      if (IntVal < 0)
        return false;
    }
    if (IntVal > IntLastVal)
      return false;
    return true;
  }
};

} // namespace vc

#endif // VC_UTILS_GENERAL_ENUMUTILS_H
