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
  static inline cl::opt<bool> OptTerminateOnFirstError{
      "genx-verify-terminate-on-first-error", cl::init(false), cl::Hidden,
      cl::desc("Terminate execution on first error found.")};

  static inline cl::opt<bool> OptTerminate{
      "genx-verify-terminate", cl::init(true), cl::Hidden,
      cl::desc(
          "Terminate execution after pass completion if any errors found.")};

  static inline cl::opt<bool> OptAllFatal{
      "genx-verify-all-fatal", cl::init(false), cl::Hidden,
      cl::desc("Ignore IsFatal::No flag used for assetions requiring spec "
               "clarification, making all of the checks fatal on failure.")};

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
  explicit GenXVerify(GenXVerifyInvariantSet IS = GenXVerifyInvariantSet::All)
      : InvariantSet(IS), ModulePass(ID) {}
  StringRef getPassName() const override;
  void getAnalysisUsage(AnalysisUsage &) const override;
  bool runOnModule(Module &) override;
  bool doInitialization(Module &);
  void visitCallInst(const CallInst &);
  void visitGlobalVariable(const GlobalVariable &);
};
