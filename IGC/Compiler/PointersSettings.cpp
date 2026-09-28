/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "llvm/Support/CommandLine.h"
#include "common/igc_regkeys.hpp"
#include <llvmWrapper/IR/LLVMContext.h>
#include <PointersSettings.h>

using namespace llvm;
namespace IGC {
static cl::opt<bool> ForceTypedPointers("typed-pointers",
                                        cl::desc("Use typed pointers (if both typed and opaque "
                                                 "are used, then opaque will be used)"),
                                        cl::init(false));

// The pointer mode must not be cached process-wide. A single process can host
// both typed and opaque pointer contexts at the same time.
static PointerMode resolvePointerMode(bool EnabledAtBuildTime) {
#if LLVM_VERSION_MAJOR >= 17
  // No typed pointer mode exists anymore, so nothing can force it off.
  (void)EnabledAtBuildTime;
  return PointerMode::Opaque;
#else
  if (ForceTypedPointers.getValue())
    return PointerMode::Typed;

  if (EnabledAtBuildTime || IGC_IS_FLAG_ENABLED(EnableOpaquePointersBackend))
    return PointerMode::Opaque;

  return PointerMode::Typed;
#endif // LLVM_VERSION_MAJOR
}

PointerMode GetDefaultPointerMode() { return resolvePointerMode(__IGC_OPAQUE_POINTERS_API_ENABLED); }

PointerMode GetComputePointerMode() { return resolvePointerMode(__IGC_OPAQUE_POINTERS_COMPUTE_ENABLED); }

bool AreOpaquePointersEnabled(llvm::LLVMContext &Ctx) { return !IGCLLVM::supportsTypedPointers(Ctx); }
} // namespace IGC
