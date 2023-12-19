/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXVerify
//===----------------------------------------------------------------------===//
//
// This pass contains GenX-specific IR validity checks.
//
#include "GenXVerify.h"

bool GenXVerify::doInitialization(Module &M) {
  Ctx = &M.getContext();
  return false;
}

StringRef GenXVerify::getPassName() const {
  return "GenX IR verification pass.";
}

void GenXVerify::getAnalysisUsage(AnalysisUsage &AU) const {
  AU.setPreservesAll();
}

bool GenXVerify::ensure(const bool Cond, const Twine &Msg, const Instruction &I,
                        const IsFatal IsFatal_) {
  if (LLVM_LIKELY(Cond))
    return true;
  if (IsFatal_ == IsFatal::Yes || OptAllFatal || !OptQuietNonFatal)
    vc::diagnose(I.getContext(),
                 DbgPrefix + "[stage:" +
                     OptStage.getParser().getOption(
                         static_cast<int>(OptStage.getValue())) +
                     "]" +
                     (IsFatal_ == IsFatal::No
                          ? " (non-fatal, spec review required)"
                          : ""),
                 Msg, DS_Warning, vc::WarningName::Generic, &I);
  if (IsFatal_ == IsFatal::Yes || OptAllFatal) {
    IsBroken = true;
    if (OptTerminationPolicy == Terminate::OnFirstError)
      terminate();
  }
  return false;
}

[[noreturn]] void GenXVerify::terminate() {
  llvm::report_fatal_error(DbgPrefix + "failed, check log for details.");
}

bool GenXVerify::runOnModule(Module &M) {
  visit(M);
  if (OptTerminationPolicy != Terminate::No && IsBroken)
    terminate();
  return false;
}

void GenXVerify::visitGlobalVariable(const GlobalVariable &GV){
    // TODO: add genx_volatile-attributed values check here.
    //       please make sure to run this check under a proper InvariantsSet to
    //       trigger it only at appropriate pipeline stage(s).
};

void GenXVerify::visitCallInst(const CallInst &CI) {
  const unsigned IntrinsicId = vc::getAnyIntrinsicID(&CI);
  switch (IntrinsicId) {
  case GenXIntrinsic::genx_rdregionf:
  case GenXIntrinsic::genx_rdregioni:
  case GenXIntrinsic::genx_rdpredregion:
  case GenXIntrinsic::genx_wrregionf:
  case GenXIntrinsic::genx_wrregioni:
  case GenXIntrinsic::genx_wrpredregion:
  case GenXIntrinsic::genx_wrconstregion:
  case GenXIntrinsic::genx_wrpredpredregion:
    verifyRegioning(CI, IntrinsicId);
    break;
  };
}

namespace llvm {
void initializeGenXVerifyPass(PassRegistry &);
} // namespace llvm

INITIALIZE_PASS_BEGIN(GenXVerify, "GenXVerify", "GenX IR verification", false,
                      false)
INITIALIZE_PASS_END(GenXVerify, "GenXVerify", "GenX IR verification", false,
                    false)

ModulePass *llvm::createGenXVerifyPass(GenXVerifyStage ValidityInvariantsSet) {
  initializeGenXVerifyPass(*PassRegistry::getPassRegistry());
  return new GenXVerify(ValidityInvariantsSet);
}
