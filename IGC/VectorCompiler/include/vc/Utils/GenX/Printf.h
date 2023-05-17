/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_PRINTF_H
#define VC_UTILS_GENX_PRINTF_H

#include <llvm/ADT/Optional.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Value.h>

#include <vector>

namespace vc {

// A name for an attribute to mark global variables which are strings that
// are used in printf builtin. Format strings and strings passed through %s
// modifier should be marked.
inline constexpr const char PrintfStringVariable[] = "VCPrintfStringVariable";
// A message to display when compiler cannot reach printf string.
inline constexpr const char PrintfStringAccessError[] =
    "Too entangled string access in printf, the compiler cannot resolve it in "
    "compile time";

// Checks whether global variable is constant and has [N x i8] type.
bool isConstantString(const llvm::GlobalVariable &GV);
bool isConstantString(const llvm::Value &V);

// Checks whether GEP takes pointer to the first element of a constant string.
bool isConstantStringFirstElementGEP(const llvm::GEPOperator &GEP);
bool isConstantStringFirstElementGEP(const llvm::Value &V);

// Returnes global string variable that \p Op points to. \p Op must be a
// pointer to i8*. Optional variant can return nullptr when there's no such
// global variable. Standard variant requires from user to provide a correct
// pointer so it points to a string.
const llvm::GlobalVariable *
getConstStringGVFromOperandOptional(const llvm::Value &Op);
llvm::GlobalVariable *getConstStringGVFromOperandOptional(llvm::Value &Op);
const llvm::GlobalVariable &getConstStringGVFromOperand(const llvm::Value &Op);
llvm::GlobalVariable &getConstStringGVFromOperand(llvm::Value &Op);

llvm::Optional<llvm::StringRef>
getConstStringFromOperandOptional(const llvm::Value &Op);
llvm::StringRef getConstStringFromOperand(const llvm::Value &Op);

// Information about a single printf argument.
struct PrintfArgInfo {
  enum TypeKind { Char, Short, Int, Long, Double, Pointer, String };
  TypeKind Type;
  // For those types where signedness is not applicable false should be set.
  bool IsSigned = false;
};

using PrintfArgInfoSeq = std::vector<PrintfArgInfo>;

// Minimal parsing of printf format string. Only types of arguments are defined.
PrintfArgInfoSeq parseFormatString(llvm::StringRef FmtStr);

// Whether \p Usr is `@llvm.vc.internal.print.format.index` inrinsic call.
bool isPrintFormatIndex(const llvm::User &Usr);

// Creates @llvm.vc.internal.print.format.index inrinsic call with \p Pointer as
// an operand. The call is inserted before \p InsertionPt.
llvm::CallInst &createPrintFormatIndex(llvm::Value &Pointer,
                                       llvm::Instruction &InsertionPt);

// There's a special case of GEP when all its users are
// vc.internal.print.format.index intrinsics. Such GEPs live until CisaBuilder
// and then handled as part of the intrinsic. This function checks whether \p
// GEP is a such GEP.
bool isLegalPrintFormatIndexGEP(const llvm::GEPOperator &GEP);
bool isLegalPrintFormatIndexGEP(const llvm::Value &V);

// Checks whether GEP with some format index users is provided.
// Unlike isLegalPrintFormatIndexGEP this function doesn't require all users to
// be format indices.
bool isPrintFormatIndexGEP(const llvm::Value &V);
bool isPrintFormatIndexGEP(const llvm::GEPOperator &V);

} // namespace vc

#endif // VC_UTILS_GENX_PRINTF_H
