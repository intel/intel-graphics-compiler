/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_ATTRIBUTES_H
#define IGCLLVM_IR_ATTRIBUTES_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/IR/Attributes.h"
#include "llvm/IR/LLVMContext.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#include "llvmWrapper/Support/ModRef.h"
#include "Probe/Assertion.h"

namespace IGCLLVM {

inline void addCapture(llvm::AttributeList &list, llvm::LLVMContext &ctx, uint8_t index,
                       IGCLLVM::CaptureComponents capture) {
#if LLVM_VERSION_MAJOR >= 22
  list = list.addParamAttribute(ctx, {index}, llvm::Attribute::getWithCaptureInfo(ctx, llvm::CaptureInfo(capture)));
#else
  if (capture == IGCLLVM::CaptureComponents::None)
    list = list.addParamAttribute(ctx, {index}, llvm::Attribute::get(ctx, llvm::Attribute::NoCapture));
  else
    IGC_ASSERT_EXIT_MESSAGE(
        false, "We only support llvm::Attribute::NoCapture/llvm::CaptureComponents::None on LLVMs below 22.");
#endif
}

} // namespace IGCLLVM

#endif // IGCLLVM_IR_ATTRIBUTES_H