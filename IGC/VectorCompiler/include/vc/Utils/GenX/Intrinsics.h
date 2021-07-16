/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_INTRINSICS_H
#define VC_UTILS_GENX_INTRINSICS_H

#include "llvm/GenXIntrinsics/GenXIntrinsics.h"

#include "Probe/Assertion.h"

#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/Value.h>

namespace llvm {
class Function;
class Module;
class Type;
} // namespace llvm

namespace vc {

// Return declaration for intrinsics with provided parameters.
// This is helper function to get genx intrinsic declaration for given
// intrinsic ID, return type and arguments.
// RetTy -- return type of new intrinsic, may be nullptr if not overloaded.
// Args -- range of Value * representing new intrinsic arguments. Each value
// must be non-null.
// Id -- new genx intrinsic ID.
// M -- module where to insert function declaration.
template <typename Range>
llvm::Function *getGenXDeclarationForIdFromArgs(llvm::Type *RetTy, Range &&Args,
                                                llvm::GenXIntrinsic::ID Id,
                                                llvm::Module &M) {
  using namespace llvm;

  IGC_ASSERT_MESSAGE(GenXIntrinsic::isGenXIntrinsic(Id),
                     "Expected genx intrinsic id");

  SmallVector<Type *, 4> Types;
  if (GenXIntrinsic::isOverloadedRet(Id)) {
    IGC_ASSERT_MESSAGE(RetTy, "Expected return type because it is overloaded");
    Types.push_back(RetTy);
  }
  for (auto &&EnumArg : llvm::enumerate(Args)) {
    if (GenXIntrinsic::isOverloadedArg(Id, EnumArg.index()))
      Types.push_back(EnumArg.value()->getType());
  }

  return GenXIntrinsic::getGenXDeclaration(&M, Id, Types);
}

} // namespace vc

#endif // VC_UTILS_GENX_INTRINSICS_H
