#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
  llvm::FunctionPass *createLoopDeadCodeEliminationPass();
  void initializeLoopDeadCodeEliminationPass(llvm::PassRegistry &);
} // End namespace IGC
