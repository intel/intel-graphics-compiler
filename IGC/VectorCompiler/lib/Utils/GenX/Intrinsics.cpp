/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/GenX/Intrinsics.h"
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

using namespace llvm;

CallInst *vc::createAnyIntrinsic(IRBuilder<> &Builder,
                                  ArrayRef<Value *> Operands,
                                  unsigned IID,
                                  ArrayRef<Type *> Types,
                                  const Twine &Name) {
  Module *M = Builder.GetInsertBlock()->getModule();
  Function *Fn = vc::getAnyDeclaration(M, IID, Types);
  IGC_ASSERT_MESSAGE(Fn, "not found correct intrinsic");
  return Builder.CreateCall(Fn, Operands, Name);
}
