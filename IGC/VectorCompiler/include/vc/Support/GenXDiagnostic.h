/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
/// Generic diagnostic info pass to not reinvent it anytime
//
//===----------------------------------------------------------------------===//

#ifndef GENXDIAGNOSTIC_H
#define GENXDIAGNOSTIC_H

#include "llvm/ADT/Twine.h"
#include "llvm/IR/DiagnosticInfo.h"
#include "llvm/IR/DiagnosticPrinter.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Value.h"

#include <string>

namespace vc {

// Diagnostic information for errors/warnings
class DiagnosticInfo : public llvm::DiagnosticInfo {
private:
  std::string Description;
  static int KindID;

  static int getKindID() {
    if (KindID == 0)
      KindID = llvm::getNextAvailablePluginDiagnosticKind();
    return KindID;
  }

public:
  // Initialize from description
  DiagnosticInfo(const llvm::Twine &Prefix, const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity Severity = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), Severity),
        Description((Prefix + ": " + Desc).str()) {}

  // Initialize with Value
  DiagnosticInfo(const llvm::Value *Val, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity Severity = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), Severity) {
    std::string Str;
    llvm::raw_string_ostream(Str) << *Val;
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  // Initialize with Type
  DiagnosticInfo(const llvm::Type *Ty, const llvm::Twine &Prefix,
                 const llvm::Twine &Desc,
                 llvm::DiagnosticSeverity Severity = llvm::DS_Error)
      : llvm::DiagnosticInfo(getKindID(), Severity) {
    std::string Str;
    llvm::raw_string_ostream(Str) << *Ty;
    Description =
        (Prefix + " failed for: <" + Str.c_str() + ">: " + Desc).str();
  }

  void print(llvm::DiagnosticPrinter &DP) const override { DP << Description; }

  static bool classof(const llvm::DiagnosticInfo *DI) {
    return DI->getKind() == getKindID();
  }
};

void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Twine &Desc,
              llvm::DiagnosticSeverity Severity = llvm::DS_Error);
void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Value *Val, const llvm::Twine &Desc,
              llvm::DiagnosticSeverity Severity = llvm::DS_Error);
void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Type *Ty, const llvm::Twine &Desc,
              llvm::DiagnosticSeverity Severity = llvm::DS_Error);

} // namespace vc

#endif // GENXDIAGNOSTIC_H
