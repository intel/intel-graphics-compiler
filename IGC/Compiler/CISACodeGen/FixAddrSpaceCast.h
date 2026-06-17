/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_FIXADDRSPACECAST_H_
#define _CISA_FIXADDRSPACECAST_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
llvm::FunctionPass *createFixAddrSpaceCastPass();
void initializeAddrSpaceCastFixingLPMPass(llvm::PassRegistry &);

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. A plain function pass (no analysis deps). name() returns the legacy
// pass argument so PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class AddrSpaceCastFixingNPM : public llvm::PassInfoMixin<AddrSpaceCastFixingNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Function &F, llvm::FunctionAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-addrspacecast-fix"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16
} // End namespace IGC

#endif // _CISA_FIXADDRSPACECAST_H_
