/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_INTRINSICS_H
#define VC_UTILS_GENX_INTRINSICS_H

#include "vc/InternalIntrinsics/InternalIntrinsics.h"

#include "Probe/Assertion.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <utility>

namespace llvm {
class Function;
class Module;
class Type;
} // namespace llvm

namespace vc {

namespace detail {
template <typename Range, typename IsIntrinsicFunc,
          typename IsOverloadedRetFunc, typename IsOverloadedArgFunc,
          typename GetDeclarationFunc>
llvm::Function *getDeclarationForIdFromArgs(llvm::Type *RetTy, Range &&Args,
                                            unsigned Id, llvm::Module &M,
                                            IsIntrinsicFunc IsIntrinsic,
                                            IsOverloadedRetFunc IsOverloadedRet,
                                            IsOverloadedArgFunc IsOverloadedArg,
                                            GetDeclarationFunc GetDeclaration) {
  using namespace llvm;

  IGC_ASSERT_MESSAGE(IsIntrinsic(Id), "Expected genx intrinsic id");

  SmallVector<Type *, 4> Types;
  if (IsOverloadedRet(Id)) {
    IGC_ASSERT_MESSAGE(RetTy, "Expected return type because it is overloaded");
    Types.push_back(RetTy);
  }
  for (auto &&EnumArg : llvm::enumerate(Args)) {
    if (IsOverloadedArg(Id, EnumArg.index()))
      Types.push_back(EnumArg.value()->getType());
  }

  return GetDeclaration(M, Id, Types);
}
} // namespace detail

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
  return detail::getDeclarationForIdFromArgs(
      RetTy, std::forward<Range>(Args), Id, M,
      [](unsigned Id) { return GenXIntrinsic::isGenXIntrinsic(Id); },
      [](unsigned Id) { return GenXIntrinsic::isOverloadedRet(Id); },
      [](unsigned Id, unsigned ArgIdx) {
        return GenXIntrinsic::isOverloadedArg(Id, ArgIdx);
      },
      [](Module &M, unsigned Id, ArrayRef<Type *> Types) {
        return GenXIntrinsic::getGenXDeclaration(
            &M, static_cast<GenXIntrinsic::ID>(Id), Types);
      });
}

// The same as \p getGenXDeclarationForIdFromArgs but for internal intrinisics.
template <typename Range>
llvm::Function *
getInternalDeclarationForIdFromArgs(llvm::Type *RetTy, Range &&Args,
                                    vc::InternalIntrinsic::ID Id,
                                    llvm::Module &M) {
  using namespace llvm;
  return detail::getDeclarationForIdFromArgs(
      RetTy, std::forward<Range>(Args), Id, M,
      [](unsigned Id) {
        return vc::InternalIntrinsic::isInternalIntrinsic(Id);
      },
      [](unsigned Id) { return vc::InternalIntrinsic::isOverloadedRet(Id); },
      [](unsigned Id, unsigned ArgIdx) {
        return vc::InternalIntrinsic::isOverloadedArg(Id, ArgIdx);
      },
      [](Module &M, unsigned Id, ArrayRef<Type *> Types) {
        return vc::InternalIntrinsic::getInternalDeclaration(
            &M, static_cast<vc::InternalIntrinsic::ID>(Id), Types);
      });
}

// Routine for creating a new llvm::Intrinsic, InternalIntrinsic or
//   GenXIntrinsic.
// Arguments:
//  \p Builder - IRBuilder, need to create a new GenXIntrinsic;
//  \p IID - Intrinsic::ID, GenXIntrinsic::ID or vc::IntrenalIntrinsic::ID - the
//    ID of intrinsic;
//  \p Types - additional input/output types for choosing right
//  GenXIntrinsic/InternalIntrinsic/Intrinsic from several with the same ID;
//  \p Operands - the expected arguments of intrinsic;
// Return:
//  A pointer to created Intrinsic/GenXIntrinsic/InternalIntrinsic, with
//    ID == IID and arguments from \p Operands;
llvm::CallInst *createAnyIntrinsic(llvm::IRBuilder<> &Builder,
                                    llvm::ArrayRef<llvm::Value *> Operands,
                                    unsigned IID,
                                    llvm::ArrayRef<llvm::Type *> Types = {},
                                    const llvm::Twine &Name = "");

} // namespace vc

#endif // VC_UTILS_GENX_INTRINSICS_H
