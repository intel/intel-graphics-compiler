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
#include <llvm/IR/IRBuilder.h>
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

// Routine for creating a new GenXIntrinsic.
// Arguments:
//  \p Builder - IRBuilder, need to create a new GenXIntrinsic;
//  \p IID - GenXIntrinsic::ID - the ID of intrinsic;
//  \p Types - additional input/output types for choosing right GenXIntrinsic
//    from several with the same ID;
//  \p Operands - the expected arguments of intrinsic;
// Return:
//  A pointer to created GenXIntrinsic, with ID == IID and arguments from
//    \p Operands;
llvm::CallInst *createGenXIntrinsic(llvm::IRBuilder<> &Builder,
                                    llvm::ArrayRef<llvm::Value *> Operands,
                                    llvm::GenXIntrinsic::ID IID,
                                    llvm::ArrayRef<llvm::Type *> Types = {},
                                    const llvm::Twine &Name = "");

} // namespace vc

#endif // VC_UTILS_GENX_INTRINSICS_H
