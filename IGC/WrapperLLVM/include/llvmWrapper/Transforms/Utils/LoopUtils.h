/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_LOOPUTILS_H
#define IGCLLVM_TRANSFORMS_UTILS_LOOPUTILS_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Transforms/Utils/LoopUtils.h"

namespace IGCLLVM {
inline bool isInnermost(llvm::Loop *L) {
  return L->isInnermost();
}
} // namespace IGCLLVM

#endif
