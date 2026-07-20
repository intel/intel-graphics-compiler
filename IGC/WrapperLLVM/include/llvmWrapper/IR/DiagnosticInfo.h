/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_DIAGNOSTICINFO_H
#define IGCLLVM_IR_DIAGNOSTICINFO_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include <llvm/Config/llvm-config.h>
#include <llvm/IR/DiagnosticInfo.h>
#include "IGC/common/LLVMWarningsPop.hpp"

namespace IGCLLVM {

// LLVM 22 changed the diagnostic-handler callback signature registered via
// LLVMContext::setDiagnosticHandlerCallBack: the DiagnosticInfo is now passed
// by pointer instead of by const reference.
//   LLVM <  22: void (*)(const DiagnosticInfo &DI, void *Context)
//   LLVM >= 22: void (*)(const DiagnosticInfo *DI, void *Context)
//
// Declare the callback's first parameter as IGCLLVM::DiagnosticInfoParamTy and
// normalize it to a const reference with getDiagnosticInfo().
#if LLVM_VERSION_MAJOR >= 22
using DiagnosticInfoParamTy = const llvm::DiagnosticInfo *;
inline const llvm::DiagnosticInfo &getDiagnosticInfo(DiagnosticInfoParamTy DI) { return *DI; }
#else
using DiagnosticInfoParamTy = const llvm::DiagnosticInfo &;
inline const llvm::DiagnosticInfo &getDiagnosticInfo(DiagnosticInfoParamTy DI) { return DI; }
#endif

} // namespace IGCLLVM

#endif
