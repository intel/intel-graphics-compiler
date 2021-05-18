/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/GenXCodeGen/GenXTarget.h"

static int initializeAll() {
  llvm::initializeGenX();
  LLVMInitializeGenXPasses();
  return 0;
}

// This will be initialized on plugin load.
// Can cause problems if library is linked at compilation time.
static const int Init = initializeAll();
