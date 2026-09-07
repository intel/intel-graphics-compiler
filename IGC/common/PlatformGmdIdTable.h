/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGC_COMMON_PLATFORM_GMD_ID_TABLE_H
#define IGC_COMMON_PLATFORM_GMD_ID_TABLE_H

#include "igfxfmid.h"

#include <string>

namespace IGC {

enum class PlatformCheckResult {
  Match,
  Unverifiable,
  UnknownGmdId,
  Mismatch,
};

struct PlatformCheckOutcome {
  PlatformCheckResult Result = PlatformCheckResult::Unverifiable;

  // Empty for Match.
  std::string Message;

  bool isFatal() const { return Result == PlatformCheckResult::Mismatch; }
};

// Compare Platform sRenderBlockID against eProductFamily and eRenderCoreFamily.
PlatformCheckOutcome checkPlatformConsistency(const PLATFORM &Platform);

} // namespace IGC

#endif // IGC_COMMON_PLATFORM_GMD_ID_TABLE_H
