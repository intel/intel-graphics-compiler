/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_SCALAR_LOOPUNROLLCONFIG_H
#define IGCLLVM_TRANSFORMS_SCALAR_LOOPUNROLLCONFIG_H

#include "llvm/Config/llvm-config.h"

namespace IGCLLVM {

// True when the legacy-wrapped LoopUnroll runs the LLVM 16+ new-PM LoopUnrollPass implementation (see
// LoopUnrollPass.cpp). Heuristics tuned around its behavior should be re-checked on every LLVM upgrade.
constexpr bool UsesNewPMLoopUnroll = LLVM_VERSION_MAJOR >= 16;

} // namespace IGCLLVM

#endif // IGCLLVM_TRANSFORMS_SCALAR_LOOPUNROLLCONFIG_H
