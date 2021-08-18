/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Support/GenXDiagnostic.h"

namespace vc {

int DiagnosticInfo::KindID = 0;

void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              const llvm::Twine &Desc, llvm::DiagnosticSeverity Severity) {
  DiagnosticInfo Diag{Prefix, Desc, Severity};
  Ctx.diagnose(Diag);
}

void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix,
              llvm::Value *Val, const llvm::Twine &Desc,
              llvm::DiagnosticSeverity Severity) {
  DiagnosticInfo Diag{Val, Prefix, Desc, Severity};
  Ctx.diagnose(Diag);
}

void diagnose(llvm::LLVMContext &Ctx, const llvm::Twine &Prefix, llvm::Type *Ty,
              const llvm::Twine &Desc, llvm::DiagnosticSeverity Severity) {
  DiagnosticInfo Diag{Ty, Prefix, Desc, Severity};
  Ctx.diagnose(Diag);
}

} // namespace vc
