/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LLVM_GENX_TARGET_INITIALIZERS_H
#define LLVM_GENX_TARGET_INITIALIZERS_H

extern "C" void LLVMInitializeGenXTargetInfo();
extern "C" void LLVMInitializeGenXTarget();
extern "C" void LLVMInitializeGenXTargetMC();
extern "C" void LLVMInitializeGenXPasses();

namespace llvm {
void initializeGenX() {
  LLVMInitializeGenXTargetInfo();
  LLVMInitializeGenXTarget();
  LLVMInitializeGenXTargetMC();
}
} // namespace llvm

#endif
