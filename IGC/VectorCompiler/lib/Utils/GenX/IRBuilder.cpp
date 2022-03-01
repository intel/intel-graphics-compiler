/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/IRBuilder.h"

#include "vc/InternalIntrinsics/InternalIntrinsics.h"
#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/TypeSize.h"
#include "vc/Utils/General/Types.h"

#include "Probe/Assertion.h"
#include "llvmWrapper/IR/DerivedTypes.h"

#include <llvm/GenXIntrinsics/GenXIntrinsics.h>

#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

using namespace llvm;

Value *vc::getGroupThreadIDForPIM(IRBuilder<> &IRB) {
  // Some constants calculation.
  // Used r0 size define the size of r0 low part needed to access TID.
  constexpr int UsedR0DWords = 4;
  const int UsedR0Bytes = UsedR0DWords * vc::DWordSize.inBytes();
  // TID is in r0.2[0:7], calculating its offset/index in bytes.
  constexpr int TIDIndexInDWords = 2;
  const int TIDIndexInBytes = TIDIndexInDWords * vc::DWordSize.inBytes() + 0;

  auto *R0Decl = GenXIntrinsic::getGenXDeclaration(
      IRB.GetInsertBlock()->getModule(), GenXIntrinsic::genx_r0,
      IGCLLVM::FixedVectorType::get(IRB.getInt32Ty(), UsedR0DWords));
  auto *R0 = IRB.CreateCall(R0Decl, None, "r0");
  auto *R0InBytes = IRB.CreateBitCast(
      R0, IGCLLVM::FixedVectorType::get(IRB.getInt8Ty(), UsedR0Bytes),
      "r0.bytes");
  return IRB.CreateExtractElement(R0InBytes, TIDIndexInBytes, "r0.tid");
}

CallInst *vc::createReadVariableRegion(GlobalVariable &Variable,
                                       const CMRegion &R, IRBuilder<> &IRB,
                                       const Twine &Name) {
  IGC_ASSERT_MESSAGE(R.ElementTy == Variable.getValueType()->getScalarType(),
                     "wrong arguments: region and variable types don't match");
  Value *Args[] = {&Variable, IRB.getInt32(R.VStride), IRB.getInt32(R.Width),
                   IRB.getInt32(R.Stride),
                   IRB.getInt32(R.getOffsetInElements())};
  Function *Decl = vc::getInternalDeclarationForIdFromArgs(
      R.getRegionType(), Args, vc::InternalIntrinsic::read_variable_region,
      *IRB.GetInsertPoint()->getModule());
  return IRB.CreateCall(Decl, Args, Name);
}

CallInst *vc::createWriteVariableRegion(GlobalVariable &Variable, Value &Input,
                                        const CMRegion &R, IRBuilder<> &IRB) {
  IGC_ASSERT_MESSAGE(R.ElementTy == Variable.getValueType()->getScalarType(),
                     "wrong arguments: region and variable types don't match");
  IGC_ASSERT_MESSAGE(R.getRegionType() == Input.getType(),
                     "wrong arguments: region and input types don't match");
  IGC_ASSERT_MESSAGE(R.is1D(), "wrong arguments: region must be 1D");
  auto *Mask = Constant::getAllOnesValue(
      vc::setScalarType(*Input.getType(), *IRB.getInt1Ty()));
  Value *Args[] = {&Variable, &Input, IRB.getInt32(R.getDstStride()),
                   IRB.getInt32(R.getOffsetInElements()), Mask};
  Function *Decl = vc::getInternalDeclarationForIdFromArgs(
      IRB.getVoidTy(), Args, vc::InternalIntrinsic::write_variable_region,
      *IRB.GetInsertPoint()->getModule());
  return IRB.CreateCall(Decl, Args);
}
