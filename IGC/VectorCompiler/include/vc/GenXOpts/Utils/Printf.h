/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_GENX_OPTS_UTILS_PRINTF_H
#define VC_GENX_OPTS_UTILS_PRINTF_H

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>

#include <vector>

namespace llvm {

Optional<StringRef> getConstStringFromOperandOptional(const Value &Op);
StringRef getConstStringFromOperand(const Value &Op);

// Information about a single printf argument.
struct PrintfArgInfo {
  enum TypeKind { Char, Short, Int, Long, Double, Pointer, String };
  TypeKind Type;
  // For those types where signedness is not applicable false should be set.
  bool IsSigned = false;
};

using PrintfArgInfoSeq = std::vector<PrintfArgInfo>;

// Minimal parsing of printf format string. Only types of arguments are defined.
PrintfArgInfoSeq parseFormatString(StringRef FmtStr);

// Whether \p Usr is @genx.print.format.index inrinsic call.
bool isPrintFormatIndex(const User &Usr);

// There's a special case of GEP when all its users are genx.print.format.index
// intrinsics. Such GEPs live until CisaBuilder and then handled as part of
// genx.print.format.index intrinsic.
// This function checks whether \p GEP is a such GEP.
bool isLegalPrintFormatIndexGEP(const GetElementPtrInst &GEP);
bool isLegalPrintFormatIndexGEP(const Value &V);

// Checks whether GEP with some format index users is provided.
// Unlike isLegalPrintFormatIndexGEP this function doesn't require all users to
// be format indices.
bool isPrintFormatIndexGEP(const User &V);
bool isPrintFormatIndexGEP(const GEPOperator &V);

} // namespace llvm

#endif // VC_GENX_OPTS_UTILS_PRINTF_H
