/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/Intrinsics.h"

using namespace llvm;
CallInst *vc::createGenXIntrinsic(IRBuilder<> &Builder,
                                  ArrayRef<llvm::Value *> Operands,
                                  GenXIntrinsic::ID IID,
                                  ArrayRef<llvm::Type *> Types,
                                  const Twine &Name) {
  Module *M = Builder.GetInsertBlock()->getModule();
  Function *Fn = GenXIntrinsic::getAnyDeclaration(M, IID, Types);
  IGC_ASSERT_MESSAGE(Fn, "not found currect intrinsic");
  return Builder.CreateCall(Fn, Operands, Name);
}
