/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _OPENCL_KERNELFUNCTIONCLONING_H_
#define _OPENCL_KERNELFUNCTIONCLONING_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {

void initializeKernelFunctionCloningLPMPass(llvm::PassRegistry &);
llvm::ModulePass *createKernelFunctionCloningPass();

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class KernelFunctionCloningNPM : public llvm::PassInfoMixin<KernelFunctionCloningNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-kernel-function-cloning"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC

#endif // _OPENCL_KERNELFUNCTIONCLONING_H_
