/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_ATTRIBUTES_H
#define IGCLLVM_IR_ATTRIBUTES_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Instructions.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Support/ModRef.h"
#include "Probe/Assertion.h"

namespace IGCLLVM {

inline llvm::AttributeList addCapture(llvm::AttributeList &list, llvm::LLVMContext &ctx, uint8_t index,
                                      IGCLLVM::CaptureComponents capture) {
#if LLVM_VERSION_MAJOR >= 22
  return list.addParamAttribute(ctx, {index}, llvm::Attribute::getWithCaptureInfo(ctx, llvm::CaptureInfo(capture)));
#else
  if (capture == IGCLLVM::CaptureComponents::None)
    return list.addParamAttribute(ctx, {index}, llvm::Attribute::get(ctx, llvm::Attribute::NoCapture));
  else
    IGC_ASSERT_EXIT_MESSAGE(
        false, "We only support llvm::Attribute::NoCapture/llvm::CaptureComponents::None on LLVMs below 22.");
#endif

  return list;
}

inline void setNoCaptureAttributeAtArgIndex(llvm::Function *F, unsigned ArgNo) {
  if (!F)
    return;

#if LLVM_VERSION_MAJOR >= 22
  llvm::LLVMContext &ctx = F->getContext();
  auto capture = llvm::Attribute::getWithCaptureInfo(ctx, llvm::CaptureInfo(IGCLLVM::CaptureComponents::None));
  F->addParamAttr(ArgNo, capture);
#else
  F->addParamAttr(ArgNo, llvm::Attribute::NoCapture);
#endif
}

inline void setNoCaptureAttributeAtArgIndex(llvm::CallInst *CI, unsigned ArgNo) {
  if (!CI)
    return;

#if LLVM_VERSION_MAJOR >= 22
  llvm::LLVMContext &ctx = CI->getContext();
  auto capture = llvm::Attribute::getWithCaptureInfo(ctx, llvm::CaptureInfo(IGCLLVM::CaptureComponents::None));
  CI->addParamAttr(ArgNo, capture);
#else
  CI->addParamAttr(ArgNo, llvm::Attribute::NoCapture);
#endif
}

inline llvm::AttributeMask typeIncompatible(llvm::Type *Ty, llvm::AttributeSet AS = llvm::AttributeSet()) {
#if LLVM_VERSION_MAJOR >= 22
  return llvm::AttributeFuncs::typeIncompatible(Ty, AS);
#else
  return llvm::AttributeFuncs::typeIncompatible(Ty);
#endif
}

} // namespace IGCLLVM

#endif // IGCLLVM_IR_ATTRIBUTES_H
