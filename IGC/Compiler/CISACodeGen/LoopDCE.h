/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
  llvm::FunctionPass *createLoopDeadCodeEliminationPass();
  void initializeLoopDeadCodeEliminationPass(llvm::PassRegistry &);

  llvm::FunctionPass* createDeadPHINodeEliminationPass();
  void initializeDeadPHINodeEliminationPass(llvm::PassRegistry &);
} // End namespace IGC
