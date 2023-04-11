/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_BASICBLOCK_H
#define IGCLLVM_IR_BASICBLOCK_H

#include <llvm/Config/llvm-config.h>
#include "llvm/IR/BasicBlock.h"

namespace IGCLLVM {
inline llvm::filter_iterator<llvm::BasicBlock::const_iterator,
  std::function<bool(const llvm::Instruction &)>>::difference_type
sizeWithoutDebug(const llvm::BasicBlock *BB) {
  return std::distance(BB->instructionsWithoutDebug().begin(),
                       BB->instructionsWithoutDebug().end());
}
} // namespace IGCLLVM

#endif
