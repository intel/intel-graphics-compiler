/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "Compiler/Optimizer/OCLBIUtils.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"
#include "IGC/common/Types.hpp"

namespace llvm {
class FunctionPass;
}

namespace IGC {
// LSC 2D block address payload field names for updating only
// (block width/height/numBlock are not updated).
enum LSC2DBlockField { BASE = 1, WIDTH = 2, HEIGHT = 3, PITCH = 4, BLOCKX = 5, BLOCKY = 6 };

llvm::FunctionPass *createLSCFuncsResolutionPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. Modeled as a module pass that loops over the defined functions (the
// seeded CodeGenContextAnalysis is module-level; IGC passes never use skipFunction). name() returns
// the legacy pass argument so PrintBefore/PrintAfter matches under the new pass manager.
class LSCFuncsResolutionNPM : public llvm::PassInfoMixin<LSCFuncsResolutionNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-lsc-funcs-translation"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // namespace IGC
