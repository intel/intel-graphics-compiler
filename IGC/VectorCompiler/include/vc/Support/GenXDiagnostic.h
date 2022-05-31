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
#include "llvm/Pass.h"

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

enum class WarningName {
  Generic,
  CMRT,
  NotAWarning,
};

// Diagnostic information for errors/warnings
class DiagnosticInfo : public llvm::DiagnosticInfo {
private:
  static const int KindID;
  std::string Description;
  llvm::DiagnosticSeverity Severity;
  WarningName WName;

  static int getKindID() { return KindID; }

public:
  // Initialize from description
  DiagnosticInfo(const llvm::Twine &Prefix, const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity SeverityIn, WarningName WNameIn)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn),
        Description((Prefix + ": " + Desc).str()), Severity(SeverityIn),
        WName(WNameIn) {}

  // Initialize with Value
  DiagnosticInfo(const llvm::Value *Val, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc, llvm::DiagnosticSeverity SeverityIn,
                 WarningName WNameIn)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn),
        WName(WNameIn) {
    std::string Str;
    printToString(Str, *Val);
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Instruction, account for debug info
  DiagnosticInfo(const llvm::Instruction *Inst, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc, llvm::DiagnosticSeverity SeverityIn,
                 WarningName WNameIn)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn),
        WName(WNameIn) {
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
                 const llvm::Twine &Desc, llvm::DiagnosticSeverity SeverityIn,
                 WarningName WNameIn)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn),
        WName(WNameIn) {
    std::string Str;
    printToString(Str, *Ty);
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Arg
  DiagnosticInfo(const llvm::Argument *Arg, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc, llvm::DiagnosticSeverity SeverityIn,
                 WarningName WNameIn)
      : llvm::DiagnosticInfo(getKindID(), SeverityIn), Severity(SeverityIn),
        WName(WNameIn) {
    auto *ArgParent = Arg->getParent();
    IGC_ASSERT(ArgParent);
    Description = (Prefix + " failed for: < Argument " +
                   llvm::Twine{Arg->getArgNo() + 1} + " in " +
                   ArgParent->getName() + ">: " + Desc)
                      .str();
  }

  void print(llvm::DiagnosticPrinter &DP) const override {
    if (Severity == llvm::DS_Error)
      llvm::report_fatal_error(llvm::StringRef(Description));
    DP << Description;
  }

  // name to use in custom diagnostic handler
  WarningName getName() const { return WName; }

  static bool classof(const llvm::DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};

// warn means warn and continue working (unless handler redefined)
template <typename... Args>
void warn(WarningName WN, llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
          const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc,
                      llvm::DS_Warning, WN};
  Ctx.diagnose(Diag);
}

// overload for generic warnings
template <typename... Args>
void warn(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
          const llvm::Twine &Desc, Args &&... args) {
  warn(WarningName::Generic, Ctx, Prefix, Desc, std::forward<Args>(args)...);
}

// overload for explicit pass
template <typename... Args>
void warn(WarningName WN, llvm::LLVMContext &Ctx, const llvm::Pass &Pass,
          const llvm::Twine &Desc, Args &&... args) {
  warn(WN, Ctx, Pass.getPassName(), Desc, std::forward<Args>(args)...);
}

// diagnose means maybe error
template <typename... Args>
void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Twine &Desc, llvm::DiagnosticSeverity DSType,
              WarningName WN, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc, DSType, WN};
  Ctx.diagnose(Diag);
}

// overload in case we are more certain
template <typename... Args>
void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc, llvm::DS_Error,
                      WarningName::NotAWarning};
  Ctx.diagnose(Diag);
}

// fatal means fatal that is why it is marked noreturn
template <typename... Args>
[[noreturn]] void fatal(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
                        const llvm::Twine &Desc, Args &&... args) {
  DiagnosticInfo Diag{std::forward<Args>(args)..., Prefix, Desc, llvm::DS_Error,
                      WarningName::NotAWarning};
  Ctx.diagnose(Diag);
  // we shall not reach this, diagnose reports fatal error
  llvm::report_fatal_error("Diag: aborted");
}

// pass name overload for fatal
template <typename... Args>
[[noreturn]] void fatal(llvm::LLVMContext &Ctx, const llvm::Pass &Pass,
                        const llvm::Twine &Desc, Args &&... args) {
  fatal(Ctx, Pass.getPassName(), Desc, std::forward<Args>(args)...);
}

// replace IR checker
inline void checkArgOperandIsConstantInt(const llvm::CallInst &CI, unsigned Idx,
                                         const llvm::Twine &ArgName) {
  auto *Op = CI.getArgOperand(Idx);
  if (llvm::isa<llvm::ConstantInt>(Op))
    return;
  fatal(CI.getContext(), "IRChecker",
        "<" + ArgName + "> is expected to be constant", &CI);
}

} // namespace vc

#endif // VC_SUPPORT_GENXDIAGNOSTIC_H
