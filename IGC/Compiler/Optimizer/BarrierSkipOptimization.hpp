/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <cstdint>

namespace IGC {

class BarrierSkipOptimization {
public:
  [[nodiscard]] static bool canSkip(uint32_t threadGroupSize, uint32_t numLanes, bool hasFusedEU);
};

} // namespace IGC
