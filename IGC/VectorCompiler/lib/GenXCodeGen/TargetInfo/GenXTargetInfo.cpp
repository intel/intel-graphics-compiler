/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "GenXTargetInfo.h"

#include "llvmWrapper/Support/TargetRegistry.h"

using namespace llvm;

Target &llvm::getTheGenXTarget32() {
  static Target TheGenXTarget32;
  return TheGenXTarget32;
}

Target &llvm::getTheGenXTarget64() {
  static Target TheGenXTarget64;
  return TheGenXTarget64;
}

extern "C" void LLVMInitializeGenXTargetInfo() {
  RegisterTarget<> X(getTheGenXTarget32(), "genx32", "Intel GenX 32-bit",
                     "genx32");
  RegisterTarget<> Y(getTheGenXTarget64(), "genx64", "Intel GenX 64-bit",
                     "genx64");
}

extern "C" void LLVMInitializeGenXTargetMC() {}
