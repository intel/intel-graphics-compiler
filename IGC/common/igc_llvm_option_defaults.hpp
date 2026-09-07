/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Support/CommandLine.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

// Preserve IGC's default of disabling instcombine code sinking on every frontend
// (replaces no-instcombine-code-sinking.patch). InitializeRegKeys() is compiled
// out on release builds, so this is invoked directly from each frontend entry
// point to cover DX/OGL/Vulkan there too. A user-provided value still wins.
inline void applyLLVMOptionDefaults() {
  auto *Sinking =
      static_cast<llvm::cl::opt<bool> *>(llvm::cl::getRegisteredOptions().lookup("instcombine-code-sinking"));
  if (Sinking && Sinking->getNumOccurrences() == 0)
    *Sinking = false;
}

} // namespace IGC
