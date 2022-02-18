/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/IRBuilder.h"

#include "vc/Utils/GenX/TypeSize.h"

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
