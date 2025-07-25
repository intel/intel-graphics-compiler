/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#pragma once

#include "GenIntrinsicEnum.h"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/ADT/None.h"
#include "llvm/IR/Function.h"
#include "common/LLVMWarningsPop.hpp"

#include <string>
#include <vector>

namespace llvm {

namespace GenISAIntrinsic {

/// Intrinsic::getName(ID) - Return the LLVM name for an intrinsic, such as
/// "llvm.ppc.altivec.lvx".
std::string getName(ID id, ArrayRef<Type *> Tys = {}, ArrayRef<Type *> OverloadedPointeeTys = {});

struct IntrinsicComments {
  const char *funcDescription;
  std::vector<const char *> outputs;
  std::vector<const char *> inputs;
};

IntrinsicComments getIntrinsicComments(ID id);

/// Intrinsic::getDeclaration(M, ID) - Create or insert an LLVM Function
/// declaration for an intrinsic, and return it.
///
/// The OverloadedTys parameter is for intrinsics with overloaded types
/// (i.e., those using iAny, fAny, vAny, or iPTRAny).  For a declaration of
/// an overloaded intrinsic, Tys must provide exactly one type for each
/// overloaded type in the intrinsic in order of dst then srcs.
///
/// For instance, consider the following overloaded function.
///    uint2 foo(size_t offset, int bar, const __global uint2 *p);
///    uint4 foo(size_t offset, int bar, const __global uint4 *p);
/// Such a function has two overloaded type parameters: dst and src2.
/// Thus the type array should two elements:
///    Type Ts[2]{int2, int2}: to resolve to the first instance.
///    Type Ts[2]{int4, int4}: to resolve to the second.
#if defined(ANDROID) || defined(__linux__)
__attribute__((visibility("default"))) Function *getDeclaration(Module *M, ID id, ArrayRef<Type *> OverloadedTys = {},
                                                                ArrayRef<Type *> OverloadedPointeeTys = {});
#else
Function *getDeclaration(Module *M, ID id, ArrayRef<Type *> OverloadedTys = {},
                         ArrayRef<Type *> OverloadedPointeeTys = {});
#endif

// Override of isIntrinsic method defined in Function.h
inline const char *getGenIntrinsicPrefix() { return "llvm.genx."; }
inline bool isIntrinsic(const Function *CF) { return (CF->getName().startswith(getGenIntrinsicPrefix())); }
ID getIntrinsicID(const Function *F, bool useContextWrapper = true);

} // namespace GenISAIntrinsic

} // namespace llvm
