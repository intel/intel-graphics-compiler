/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#include "vc/Utils/GenX/IntrinsicsWrapper.h"

using namespace llvm;

unsigned vc::getAnyIntrinsicID(const Function *F) {
  IGC_ASSERT(F);
  if (GenXIntrinsic::isGenXNonTrivialIntrinsic(F))
    return GenXIntrinsic::getGenXIntrinsicID(F);
  if (InternalIntrinsic::isInternalNonTrivialIntrinsic(F))
    return InternalIntrinsic::getInternalIntrinsicID(F);

  unsigned IID = F->getIntrinsicID();
  // TODO: return the biggest constant from intrinsics ID
  // currently saved the expected behaviour
  // to make sure assert
  // ID != GenXIntrinsics::not_any_intrinsic
  // works right
  if (IID == Intrinsic::not_intrinsic)
    return GenXIntrinsic::not_any_intrinsic;
  return IID;
}

unsigned vc::getAnyIntrinsicID(const llvm::Value *V) {
  if (!V)
    return GenXIntrinsic::not_any_intrinsic;
  const CallInst *CI = dyn_cast<CallInst>(V);
  if (!CI)
    return GenXIntrinsic::not_any_intrinsic;
  Function *Callee = CI->getCalledFunction();
  // TODO: may be this is error?
  // nullptr = this is an indirect function
  // invocation or the function signature
  // does not match the call signature
  if (!Callee)
    return GenXIntrinsic::not_any_intrinsic;

  return getAnyIntrinsicID(Callee);
}
