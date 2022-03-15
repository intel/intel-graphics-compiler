/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENX_INTRINSICS_H
#define VC_UTILS_GENX_INTRINSICS_H

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/TypeSize.h"

#include "Probe/Assertion.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/STLExtras.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>

#include <type_traits>
#include <utility>

namespace llvm {
class Function;
class Module;
class Type;
} // namespace llvm

namespace vc {

namespace detail {

// Returns zero extended value of a call inst \p CI constant operand with
// \p OpIdx index.
inline int getConstIntOperand(const llvm::CallInst &CI, int OpIdx) {
  return llvm::cast<llvm::ConstantInt>(CI.getOperand(OpIdx))->getZExtValue();
}

template <bool IsConst> class ReadVariableRegionImpl {
  using MaybeConstCallInst =
      std::conditional_t<IsConst, const llvm::CallInst, llvm::CallInst>;
  using MaybeConstGlobalVariable =
      std::conditional_t<IsConst, const llvm::GlobalVariable,
                         llvm::GlobalVariable>;
  MaybeConstCallInst *CI;

public:
  enum Operand {
    VariableIdx,
    VStrideIdx,
    WidthIdx,
    StrideIdx,
    OffsetIdx,
    NumOperands
  };

  ReadVariableRegionImpl(MaybeConstCallInst &CIIn) : CI{&CIIn} {
    IGC_ASSERT_MESSAGE(vc::InternalIntrinsic::getInternalIntrinsicID(CI) ==
                           vc::InternalIntrinsic::read_variable_region,
                       "expected intrinsic wasn't provided to the constructor");
    IGC_ASSERT_MESSAGE(getVariable().getValueType() ==
                           CI->getType()->getScalarType(),
                       "Variable operand and return types don't match");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(VStrideIdx)),
                       "VStride operand must be a constant");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(WidthIdx)),
                       "Width operand must be a constant");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(StrideIdx)),
                       "Stride operand must be a constant");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(OffsetIdx)),
                       "Offset operand must be a constant");
  }

  // Casting back to llvm::CallInst.
  MaybeConstCallInst &getCallInst() { return *CI; }
  const llvm::CallInst &getCallInst() const { return *CI; }

  operator MaybeConstCallInst &() { return getCallInst(); }
  operator const llvm::CallInst &() const { return getCallInst(); }

  // Gets predefined vISA variable represented as llvm::GlobalVariable.
  MaybeConstGlobalVariable &getVariable() {
    return *llvm::cast<llvm::GlobalVariable>(CI->getOperand(VariableIdx));
  }

  // Gets predefined vISA variable represented as llvm::GlobalVariable.
  const llvm::GlobalVariable &getVariable() const {
    return *llvm::cast<llvm::GlobalVariable>(CI->getOperand(VariableIdx));
  }

  // Region properties accessors.
  int getVStride() const { return getConstIntOperand(*CI, VStrideIdx); }
  int getWidth() const { return getConstIntOperand(*CI, WidthIdx); }
  int getStride() const { return getConstIntOperand(*CI, StrideIdx); }
  int getOffsetInElements() const { return getConstIntOperand(*CI, OffsetIdx); }

  vc::TypeSizeWrapper getOffset(llvm::DataLayout *DL = nullptr) const {
    return getOffsetInElements() * vc::getTypeSize(getElementType(), DL);
  }

  // Gets predefined variable element type.
  llvm::Type *getElementType() const { return CI->getType()->getScalarType(); }
};

template <bool IsConst> class WriteVariableRegionImpl {
  using MaybeConstCallInst =
      std::conditional_t<IsConst, const llvm::CallInst, llvm::CallInst>;
  using MaybeConstGlobalVariable =
      std::conditional_t<IsConst, const llvm::GlobalVariable,
                         llvm::GlobalVariable>;
  using MaybeConstValue =
      std::conditional_t<IsConst, const llvm::Value, llvm::Value>;
  MaybeConstCallInst *CI;

public:
  enum Operand {
    VariableIdx,
    InputIdx,
    StrideIdx,
    OffsetIdx,
    MaskIdx,
    NumOperands
  };

  WriteVariableRegionImpl(MaybeConstCallInst &CIIn) : CI{&CIIn} {
    IGC_ASSERT_MESSAGE(vc::InternalIntrinsic::getInternalIntrinsicID(CI) ==
                           vc::InternalIntrinsic::write_variable_region,
                       "expected intrinsic wasn't provided to the constructor");
    IGC_ASSERT_MESSAGE(getVariable().getValueType() ==
                           getInput().getType()->getScalarType(),
                       "Variable and input operand types don't match");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(StrideIdx)),
                       "Stride operand must be a constant");
    IGC_ASSERT_MESSAGE(llvm::isa<llvm::ConstantInt>(CI->getOperand(OffsetIdx)),
                       "Offset operand must be a constant");
  }

  // Casting back to llvm::CallInst.
  MaybeConstCallInst &getCallInst() { return *CI; }
  const llvm::CallInst &getCallInst() const { return *CI; }

  operator MaybeConstCallInst &() { return getCallInst(); }
  operator const llvm::CallInst &() const { return getCallInst(); }

  // Gets predefined vISA variable represented as llvm::GlobalVariable.
  MaybeConstGlobalVariable &getVariable() {
    return *llvm::cast<llvm::GlobalVariable>(CI->getOperand(VariableIdx));
  }

  // Gets predefined vISA variable represented as llvm::GlobalVariable.
  const llvm::GlobalVariable &getVariable() const {
    return *llvm::cast<llvm::GlobalVariable>(CI->getOperand(VariableIdx));
  }

  // Gets value that is being written to the predefined variable.
  MaybeConstValue &getInput() { return *CI->getOperand(InputIdx); }
  const llvm::Value &getInput() const { return *CI->getOperand(InputIdx); }

  // Gets predecation mask.
  MaybeConstValue &getMask() { return *CI->getOperand(MaskIdx); }
  const llvm::Value &getMask() const { return *CI->getOperand(MaskIdx); }

  // Region properties accessors.
  int getStride() const { return getConstIntOperand(*CI, StrideIdx); }
  int getOffsetInElements() const { return getConstIntOperand(*CI, OffsetIdx); }

  vc::TypeSizeWrapper getOffset(llvm::DataLayout *DL = nullptr) const {
    return getOffsetInElements() * vc::getTypeSize(getElementType(), DL);
  }

  // Gets predefined variable element type.
  llvm::Type *getElementType() const {
    return getInput().getType()->getScalarType();
  }
};

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

// A wrapper class for CallInst with @llvm.vc.internal.read.variable.region
// intrinsic. Provides convenient accessors for the intrinsic specific operands
// and properties.
using ReadVariableRegion = detail::ReadVariableRegionImpl</* IsConst=*/false>;
// The same as above but for const CallInst case.
using ReadVariableRegionConst =
    detail::ReadVariableRegionImpl</* IsConst=*/true>;

// A wrapper class for CallInst with @llvm.vc.internal.write.variable.region
// intrinsic. Provides convenient accessors for the intrinsic specific operands
// and properties.
using WriteVariableRegion = detail::WriteVariableRegionImpl</* IsConst=*/false>;
// The same as above but for const CallInst case.
using WriteVariableRegionConst =
    detail::WriteVariableRegionImpl</* IsConst=*/true>;

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
