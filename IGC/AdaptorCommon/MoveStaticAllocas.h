/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CISACodeGen/helper.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/MetaDataApi/MetaDataApi.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
/*
This pass searches for "static" alloca instructions in function that are not
in the entry block and moves them. This prevents llvm from inserting
@llvm.stacksave and @llvm.stackrestore that are not handled by IGC.
*/
// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class MoveStaticAllocas {
public:
  MoveStaticAllocas() {}
  ~MoveStaticAllocas() {}

  static llvm::StringRef getPassName() { return "MoveStaticAllocasPass"; }

  bool run(llvm::Function &F);
};

// Legacy Pass Manager wrapper.
class MoveStaticAllocasLPM : public llvm::FunctionPass {
public:
  static char ID;

  MoveStaticAllocasLPM();
  ~MoveStaticAllocasLPM() {}

  llvm::StringRef getPassName() const override { return MoveStaticAllocas::getPassName(); }

  bool runOnFunction(llvm::Function &F) override { return MoveStaticAllocas().run(F); }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class MoveStaticAllocasNPM : public llvm::PassInfoMixin<MoveStaticAllocasNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-move-static-allocas"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
