/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_BASICBLOCK_H
#define IGCLLVM_IR_BASICBLOCK_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/Config/llvm-config.h>
#include "llvm/IR/BasicBlock.h"
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {
inline llvm::filter_iterator<llvm::BasicBlock::const_iterator,
                             std::function<bool(const llvm::Instruction &)>>::difference_type
sizeWithoutDebug(const llvm::BasicBlock *BB) {
  return std::distance(BB->instructionsWithoutDebug().begin(), BB->instructionsWithoutDebug().end());
}

inline void pushFrontInstruction(llvm::BasicBlock *BB, llvm::Instruction *I) {
#if LLVM_VERSION_MAJOR < 16
  BB->getInstList().push_front(I);
#else
  I->insertInto(BB, BB->begin());
#endif
}

inline void pushBackInstruction(llvm::BasicBlock *BB, llvm::Instruction *I) {
#if LLVM_VERSION_MAJOR < 16
  BB->getInstList().push_back(I);
#else
  I->insertInto(BB, BB->end());
#endif
}

inline void popBackInstruction(llvm::BasicBlock *BB) {
#if LLVM_VERSION_MAJOR < 16
  BB->getInstList().pop_back();
#else
  BB->back().eraseFromParent();
#endif
}

inline void splice(llvm::BasicBlock *to, llvm::BasicBlock::iterator it, llvm::BasicBlock *from,
                   llvm::BasicBlock::iterator start, llvm::BasicBlock::iterator end) {
#if LLVM_VERSION_MAJOR < 16
  to->getInstList().splice(to->begin(), from->getInstList(), start, end);
#else
  to->splice(to->begin(), from, start, end);
#endif
}
} // namespace IGCLLVM

#endif
