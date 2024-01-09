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

  static inline cl::opt<GenXVerifyStage> OptStage{
      "genx-verify-stage",
      cl::desc("Check for this pipeline stage-dependent invariant set."),
      cl::init(GenXVerifyStage::Default_), cl::NotHidden,
      cl::values(
#define GENX_VERIFY_STAGE(c_, s_, d_) clEnumValN(GenXVerifyStage::c_, s_, d_),
#include "GenXVerifyStages.inc.h"
#undef GENX_VERIFY_STAGE
          clEnumValN(
              GenXVerifyStage::Default_, "default",
              "refers to the verification set for latest pipeline stages"))};

  static inline cl::opt<Terminate> OptTerminationPolicy{
      "genx-verify-terminate", cl::desc("Execution termination policy."),
      cl::init(Terminate::No), cl::NotHidden,
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
  LLVMContext *Ctx = nullptr;
  bool IsBroken = false;
  GenXVerifyStage Stage;
  enum class IsFatal { No = 0, Yes = 1 };

  void verifyRegioning(const CallInst &, const unsigned);
  bool ensure(const bool Cond, const Twine &Msg, const Instruction &I,
              const IsFatal IsFatal_ = IsFatal::Yes);
  [[noreturn]] static void terminate();

public:
  explicit GenXVerify(GenXVerifyStage Stage_ = OptStage)
      : Stage(Stage_), ModulePass(ID) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &) const override;
  bool runOnModule(Module &) override;
  bool doInitialization(Module &) override;
  void visitCallInst(const CallInst &);
  void visitGlobalVariable(const GlobalVariable &);
};
