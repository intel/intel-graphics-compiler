/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_BASICBLOCKUTILS_H
#define IGCLLVM_TRANSFORMS_UTILS_BASICBLOCKUTILS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Value.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline void ReplaceInstWithValue([[maybe_unused]] llvm::BasicBlock &BB, llvm::BasicBlock::iterator &BI,
                                 llvm::Value *V) {
#if LLVM_VERSION_MAJOR < 16
  llvm::ReplaceInstWithValue(BB.getInstList(), BI, V);
#else
  llvm::ReplaceInstWithValue(BI, V);
#endif
}
} // namespace IGCLLVM

#endif
