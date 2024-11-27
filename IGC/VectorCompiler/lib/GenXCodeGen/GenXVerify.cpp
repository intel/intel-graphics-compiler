/*========================== begin_copyright_notice ============================

Copyright (C) 2023-2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
// GenXVerify
//===----------------------------------------------------------------------===//
//
// This pass contains GenX-specific IR validity checks.
//
#include "GenXVerify.h"
#if LLVM_VERSION_MAJOR >= 16
#include "vc/GenXCodeGen/GenXVerify.h"
#endif

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

bool GenXVerify::ensure(const bool Cond, const Twine &Msg, const Value &V,
                        const IsFatal IsFatal_) {
  if (LLVM_LIKELY(Cond))
    return true;
  if (IsFatal_ == IsFatal::Yes || OptAllFatal || !OptQuietNonFatal)
    vc::diagnose(V.getContext(),
                 DbgPrefix + "[stage:" +
                     OptStage.getParser().getOption(
                         static_cast<int>(OptStage.getValue())) +
                     "]" +
                     (IsFatal_ == IsFatal::No
                          ? " (non-fatal, spec review required)"
                          : ""),
                 Msg, DS_Warning, vc::WarningName::Generic, &V);
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
  if (Stage == GenXVerifyStage::PostIrAdaptors)
    for (const auto &GV : M.globals())
      visitGlobalVariable(GV);
  if (OptTerminationPolicy != Terminate::No && IsBroken)
    terminate();
  return false;
}

void GenXVerify::visitGlobalVariable(const GlobalVariable &GV){
  if (!GV.hasAttribute(genx::FunctionMD::GenXVolatile))
    return;
  ensure(GV.getAddressSpace() == vc::AddrSpace::Private,
         "a volatile variable must reside in private address space", GV);
  auto InvalidUser = llvm::find_if(GV.users(), [](const User *U) {
    const auto *I = dyn_cast<Instruction>(genx::peelBitCastsWhileSingleUserChain(U));
    return !I || !(genx::isAVStore(I) || genx::isAVLoad(I));
  });
  if (InvalidUser == GV.user_end())
    return;
  ensure(false,
         "a volatile variable may only be used in genx.vload/genx.vstore "
         "intrinsics and volatile loads/stores instructions",
         **InvalidUser);
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

#if LLVM_VERSION_MAJOR >= 16
PreservedAnalyses GenXVerifyPass::run(llvm::Module &M,
                                      llvm::AnalysisManager<llvm::Module> &) {
  GenXVerify GenXVer;
  GenXVer.runOnModule(M);
  return PreservedAnalyses::all();
}
#endif
