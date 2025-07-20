/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Support/CommandLine.h"
#include "common/igc_regkeys.hpp"
#include <PointersSettings.h>
#include <optional>

using namespace llvm;
namespace IGC {
static cl::opt<bool> ForceTypedPointers("typed-pointers",
                                        cl::desc("Use typed pointers (if both typed and opaque "
                                                 "are used, then opaque will be used)"),
                                        cl::init(false));

std::optional<bool> OpaquePointersCache;

bool AreOpaquePointersEnabled() {
  if (OpaquePointersCache.has_value())
    return OpaquePointersCache.value();

  bool enableOpaquePointers = __IGC_OPAQUE_POINTERS_API_ENABLED || IGC_IS_FLAG_ENABLED(EnableOpaquePointersBackend);

  if (ForceTypedPointers.getValue()) {
    enableOpaquePointers = false;
  }

  OpaquePointersCache = enableOpaquePointers;

  return enableOpaquePointers;
}
} // namespace IGC
