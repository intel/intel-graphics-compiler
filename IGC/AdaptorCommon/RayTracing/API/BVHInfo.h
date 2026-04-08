/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <stdint.h>
#include <type_traits>

namespace IGC {
struct BVHInfo {
       // This is provided so that:
       // 1) No load of the offset is required (since we have it right here)
       // 2) A branch is not needed to guard the load if the bvh ptr is null
       // 3) The removed branch will enable better vectorization in many cases
  size_t offset = 0;
  uint32_t hasFixedOffset = 0;
  uint32_t uses64Bit = 0;

  inline bool operator==(const BVHInfo &RHS) const {
    return (uses64Bit == RHS.uses64Bit && hasFixedOffset == RHS.hasFixedOffset && offset == RHS.offset);
  }
};
} // namespace IGC
