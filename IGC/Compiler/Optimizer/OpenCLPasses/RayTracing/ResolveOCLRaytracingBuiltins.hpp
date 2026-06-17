/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "IGC/common/StringMacros.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/PassManager.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/CodeGenPublic.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "visa/include/visa_igc_common_header.h"

namespace llvm {
class RTBuilder;
}

namespace IGC {

// Shared implementation. Holds the logic and is used by both the legacy and the
// new-pass-manager wrappers below; it is not itself an llvm::Pass.
class ResolveOCLRaytracingBuiltins : public llvm::InstVisitor<ResolveOCLRaytracingBuiltins> {

public:
  ResolveOCLRaytracingBuiltins() {}
  ~ResolveOCLRaytracingBuiltins() {}

  static llvm::StringRef getPassName() { return "ResolveOCLRaytracingBuiltins"; }

  bool run(llvm::Module &M, CodeGenContext *pCtx);

  void visitCallInst(llvm::CallInst &callInst);

  void handleGetRtStack(llvm::CallInst &callInst);
  void handleGetThreadBTDStack(llvm::CallInst &callInst);
  void handleGetGlobalBTDStack(llvm::CallInst &callInst);
  void handleDispatchTraceRayQuery(llvm::CallInst &callInst);
  void handleRTSync(llvm::CallInst &callInst);
  void handleGetRTGlobalBuffer(llvm::CallInst &callInst);
  void handleInitRayQuery(llvm::CallInst &callInst);
  void handleUpdateRayQuery(llvm::CallInst &callInst);
  void handleQuery(llvm::CallInst &callInst);
  void handleTraversalDoneFail(llvm::CallInst &callInst);
  void handlePostProcessRayQueryReturn(llvm::CallInst &callInst);

private:
  CodeGenContext *m_pCtx = nullptr;
  std::vector<llvm::CallInst *> m_callsToReplace;
  llvm::RTBuilder *m_builder = nullptr;

  void handleGetBTDStack(llvm::CallInst &callInst, const bool isGlobal);

  void defineOpaqueTypes();

  llvm::Value *getIntrinsicValue(llvm::GenISAIntrinsic::ID intrinsicId,
                                 llvm::ArrayRef<llvm::Value *> args = llvm::ArrayRef<llvm::Value *>());
};

// Legacy Pass Manager wrapper.
class ResolveOCLRaytracingBuiltinsLPM : public llvm::ModulePass {
public:
  static char ID;

  ResolveOCLRaytracingBuiltinsLPM();
  ~ResolveOCLRaytracingBuiltinsLPM() {}

  virtual llvm::StringRef getPassName() const override { return ResolveOCLRaytracingBuiltins::getPassName(); }

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override { AU.addRequired<CodeGenContextWrapper>(); }

  virtual bool runOnModule(llvm::Module &M) override {
    return ResolveOCLRaytracingBuiltins().run(M, getAnalysis<CodeGenContextWrapper>().getCodeGenContext());
  }
};

#if LLVM_VERSION_MAJOR >= 16
// New Pass Manager wrapper. name() returns the legacy pass argument so that
// PrintBefore/PrintAfter=<pass argument> matches under the new pass manager.
class ResolveOCLRaytracingBuiltinsNPM : public llvm::PassInfoMixin<ResolveOCLRaytracingBuiltinsNPM> {
public:
  llvm::PreservedAnalyses run(llvm::Module &M, llvm::ModuleAnalysisManager &AM);
  static llvm::StringRef name() { return "igc-resolve-ocl-raytracing-builtins"; }
  static bool isRequired() { return true; }
};
#endif // LLVM_VERSION_MAJOR >= 16

} // namespace IGC
