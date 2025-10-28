/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>

namespace IGC{
  llvm::FunctionPass *createGenericNullPtrPropagationPass();
  void initializeGenericNullPtrPropagationPass(llvm::PassRegistry &);
}
