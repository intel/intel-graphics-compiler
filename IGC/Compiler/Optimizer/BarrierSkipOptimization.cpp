/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/InstIterator.h"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/IGCPassSupport.h"
#include "Compiler/CISACodeGen/helper.h"
#include "BarrierSkipOptimization.hpp"
#include "common/igc_regkeys.hpp"

using namespace llvm;

namespace IGC {

// Static utility to determine if a barrier can be safely skipped.
// Caller must ensure threadGroupSize > 0.
bool BarrierSkipOptimization::canSkip(uint32_t threadGroupSize, uint32_t numLanes, bool hasFusedEU) {
  IGC_ASSERT_MESSAGE(threadGroupSize > 0, "threadGroupSize must be non-zero");

  // Thread group fits in a single SIMD width where all threads execute in lockstep
  if (threadGroupSize <= numLanes) {
    return true;
  }

  // Fused EU: thread group fits in 2x SIMD lanes.
  // Both EUs in a fused pair share the same instruction pointer, so they
  // cannot diverge - no barrier synchronization needed.
  if (hasFusedEU && threadGroupSize == (2 * numLanes)) {
    return true;
  }

  return false;
}

} // namespace IGC
