/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// Similar to std::optional but simple and mostly intended for simple types,
// this is useful in interfacing with the UMD. See Array.h comments for
// rationale.
//
//===----------------------------------------------------------------------===//

#pragma once

#include <type_traits>
#include <assert.h>
#include "AdaptorCommon/API/igc.h"

namespace Interface {

template <typename T> class Optional {
  static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>, "simple types for now!");

  T Val;
  bool Valid = false;

public:
  Optional() : Valid(false) {}
  Optional(T V) : Val(V), Valid(true) {}
  Optional(const Optional &RHS) = default;
  Optional &operator=(const Optional &RHS) = default;
  Optional(Optional &&RHS) = default;
  Optional &operator=(Optional &&RHS) = default;

  bool has_value() const { return Valid; }

  explicit operator bool() const { return has_value(); }

  const T *operator->() const {
    assert(has_value());
    return &Val;
  }
  T *operator->() {
    assert(has_value());
    return &Val;
  }
  const T &operator*() const {
    assert(has_value());
    return Val;
  }
  T &operator*() {
    assert(has_value());
    return Val;
  }

  void reset() { Valid = false; }
};

template <typename T> inline bool operator==(const Optional<T> &, const Optional<T> &) { return true; }

} // namespace Interface
