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

#include "GenXIntrinsics.h"
#include "GenXTargetMachine.h"
#include "GenXUtil.h"

#include "vc/Support/GenXDiagnostic.h"

#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Verifier.h>

#define DEBUG_TYPE "GENX_VERIFY"

using namespace llvm;
using namespace genx;

class GenXVerify : public ModulePass,
                   public IDMixin<GenXVerify>,
                   public InstVisitor<GenXVerify> {
private:
  enum class Terminate { No = 0, Yes, OnFirstError };
  static_assert(Terminate::No < Terminate::Yes &&
                Terminate::Yes < Terminate::OnFirstError);

  static inline cl::opt<GenXVerifyInvariantSet> OptInvSet{
      "genx-verify-set",
      cl::desc("Check for this pipeline-dependent invariant set."),
      cl::init(GenXVerifyInvariantSet::Default_), cl::NotHidden,
      cl::values(
          clEnumValN(GenXVerifyInvariantSet::PostSPIRVReader,
                     "post-spirv-reader",
                     "Checks valid for early stage of IR acquired right after "
                     "SPIRV->LLVM translation."),
          clEnumValN(GenXVerifyInvariantSet::PostIrAdaptors, "post-ir-adaptors",
                     "Checks valid after IR adaptors run."),
          clEnumValN(GenXVerifyInvariantSet::PostGenXLowering,
                     "post-genx-lowering",
                     "Checks valid after GenXLowering pass."),
          clEnumValN(GenXVerifyInvariantSet::PostGenXLegalization,
                     "post-genx-legalization",
                     "Checks valid after GenXLegalization pass."),
          clEnumValN(
              GenXVerifyInvariantSet::Default_, "default",
              "Default checks set (must be a checks set used for the late "
              "pipeline stages)"))};

  static inline cl::opt<Terminate> OptTerminationPolicy{
      "genx-verify-terminate", cl::desc("Execution termination policy."),
      cl::init(Terminate::Yes), cl::NotHidden,
      cl::values(
          clEnumValN(Terminate::No, "no",
                     "Do not terminate if error(s) found."),
          clEnumValN(Terminate::Yes, "yes",
                     "Terminate after pass completion if any errors found."),
          clEnumValN(Terminate::OnFirstError, "first",
                     "Terminate on first error found."))};

  static inline cl::opt<bool> OptAllFatal{
      "genx-verify-all-fatal", cl::init(false), cl::NotHidden,
      cl::desc("Ignore IsFatal::No flag used for assetions requiring spec "
               "clarification, making all of the checks fatal on failure.")};

  static inline cl::opt<bool> OptQuietNonFatal{
      "genx-verify-quiet-non-fatal", cl::init(true), cl::NotHidden,
      cl::desc("Do not display non-fatal warnings.")};

  static inline const StringRef DbgPrefix = "GenXVerify";
  LLVMContext *Ctx;
  bool IsBroken = false;
  GenXVerifyInvariantSet InvariantSet;
  enum class IsFatal { No = 0, Yes = 1 };

  void verifyRegioning(const CallInst &, const unsigned);
  bool ensure(const bool Cond, const Twine &Msg, const Instruction &I,
              const IsFatal IsFatal_ = IsFatal::Yes);
  [[noreturn]] static void terminate();

public:
  explicit GenXVerify(GenXVerifyInvariantSet IS = OptInvSet)
      : InvariantSet(IS), ModulePass(ID) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &) const override;
  bool runOnModule(Module &) override;
  bool doInitialization(Module &);
  void visitCallInst(const CallInst &);
  void visitGlobalVariable(const GlobalVariable &);
};
