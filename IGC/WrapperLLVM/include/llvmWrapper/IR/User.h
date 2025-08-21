/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_USER_H
#define IGCLLVM_IR_USER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/User.h"

namespace IGCLLVM {
inline bool isDroppable(llvm::User *U) {
  return U->isDroppable();
}
} // namespace IGCLLVM

#endif // IGCLLVM_IR_USER_H