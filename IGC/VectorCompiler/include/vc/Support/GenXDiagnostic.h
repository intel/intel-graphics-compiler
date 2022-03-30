/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// Generic diagnostic info pass to not reinvent it anytime
//
//===----------------------------------------------------------------------===//

#ifndef VC_SUPPORT_GENXDIAGNOSTIC_H
#define VC_SUPPORT_GENXDIAGNOSTIC_H

#include <string>

#include "llvm/ADT/Twine.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"

#include "Probe/Assertion.h"

namespace vc {

// Prints \p V to string \p Str.
template <typename T> void printToString(std::string &Str, T &V) {
#if LLVM_VERSION_MAJOR < 10
  llvm::raw_string_ostream StrStream{Str};
  StrStream << V;
#else
  llvm::raw_string_ostream{Str} << V;
#endif
}

// Diagnostic information for errors/warnings
class DiagnosticInfo : public llvm::DiagnosticInfo {
private:
  std::string Description;
  static const int KindID;
  llvm::DiagnosticSeverity Severity;

  static int getKindID() { return KindID; }

public:
  // Initialize from description
  DiagnosticInfo(const llvm::Twine &Prefix, const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn),
        Description((Prefix + ": " + Desc).str()), Severity(SeverityIn) {}

  // Initialize with Value
  DiagnosticInfo(const llvm::Value *Val, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn) {
    std::string Str;
    printToString(Str, *Val);
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Instruction, account for debug info
  DiagnosticInfo(const llvm::Instruction *Inst, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn) {
    std::string Str;
    printToString(Str, *Inst);

    auto DL = Inst->getDebugLoc();
    if (DL) {
      llvm::StringRef Filename = DL.get()->getFilename();
      unsigned Line = DL.getLine();
      unsigned Col = DL.getCol();
      Str += (llvm::Twine(" ") + llvm::Twine(Filename) + " : " +
              llvm::Twine(Line) + " : " + llvm::Twine(Col))
                 .str();
    }

    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Type
  DiagnosticInfo(const llvm::Type *Ty, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn) {
    std::string Str;
    printToString(Str, *Ty);
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Arg
  DiagnosticInfo(const llvm::Argument *Arg, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn) {
    auto *ArgParent = Arg->getParent();
    IGC_ASSERT(ArgParent);
    Description = (Prefix + " failed for: < Argument " +
                   llvm::Twine{Arg->getArgNo() + 1} + " in " +
                   ArgParent->getName() + ">: " + Desc)
                      .str();
  }

  void print(llvm::DiagnosticPrinter &DP) const override {
    if (Severity == llvm::DS_Error)
      llvm::report_fatal_error(Description);
    DP << Description;
  }

  static bool classof(const llvm::DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};

// warn means warn and continue working (unless handler redefined)
template <typename... Args>
void warn(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
          const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc,
                      llvm::DS_Warning};
  Ctx.diagnose(Diag);
}

// diagnose means error but continue working
template <typename... Args>
void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc,
                      llvm::DS_Error};
  Ctx.diagnose(Diag);
}

// fatal means fatal
template <typename... Args>
[[noreturn]] void fatal(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
                        const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc,
                      llvm::DS_Error};
  Ctx.diagnose(Diag);
  // we shall not reach this, diagnose reports fatal error
  llvm::report_fatal_error("Diag: aborted");
}

template <typename Diag>
struct IRChecker {
  static void argOperandIsConstantInt(const llvm::CallInst &CI, unsigned Idx,
                                      const llvm::Twine &ArgName) {
    auto *Op = CI.getArgOperand(Idx);
    if (llvm::isa<llvm::ConstantInt>(Op))
      return;
    Diag Err(&CI, "<" + ArgName + "> is expected to be constant",
             llvm::DS_Error);
    CI.getContext().diagnose(Err);
  }
};

} // namespace vc

#endif // VC_SUPPORT_GENXDIAGNOSTIC_H
