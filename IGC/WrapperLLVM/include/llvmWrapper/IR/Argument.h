/*========================== begin_copyright_notice ============================

Copyright (C) 2024 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_ARGUMENT_H
#define IGCLLVM_IR_ARGUMENT_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/Type.h"
#include "llvm/IR/Argument.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#include "Probe/Assertion.h"

namespace IGCLLVM {

inline bool hasByRefAttr(const llvm::Argument *arg) { return arg->hasByRefAttr(); }

inline llvm::Type *getParamByRefType(const llvm::Argument *arg) { return arg->getParamByRefType(); }
inline void setParamByRefType([[maybe_unused]] llvm::Argument *arg, [[maybe_unused]] llvm::Type *param) {
  arg->addAttr(llvm::Attribute::get(arg->getParent()->getContext(), llvm::Attribute::ByRef, param));
}

inline llvm::Type *getArgAttrEltTy(const llvm::Argument *Arg) {
  llvm::AttributeSet ParamAttrs = Arg->getParent()->getAttributes().getParamAttrs(Arg->getArgNo());
  if (llvm::Type *ByValTy = ParamAttrs.getByValType())
    return ByValTy;
  if (llvm::Type *ByRefTy = ParamAttrs.getByRefType())
    return ByRefTy;
  if (llvm::Type *PreAllocTy = ParamAttrs.getPreallocatedType())
    return PreAllocTy;
  if (llvm::Type *InAllocaTy = ParamAttrs.getInAllocaType())
    return InAllocaTy;
  if (llvm::Type *SRetTy = ParamAttrs.getStructRetType())
    return SRetTy;
  return nullptr;
}

} // namespace IGCLLVM

#endif // IGCLLVM_IR_ARGUMENT_H