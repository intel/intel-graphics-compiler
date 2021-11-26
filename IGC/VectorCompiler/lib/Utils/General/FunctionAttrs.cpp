/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "vc/Utils/General/FunctionAttrs.h"

#include "Probe/Assertion.h"

void vc::transferDISubprogram(llvm::Function &From, llvm::Function &To) {
  auto *DISp = From.getSubprogram();
  To.setSubprogram(DISp);
  // DISubprogram must be unique to the module.
  // We preserve IR correctness by detaching DISubprogram node from the original
  // function
  From.setSubprogram(nullptr);
}

void vc::transferNameAndCCWithNewAttr(const llvm::AttributeList Attrs,
                                      llvm::Function &From,
                                      llvm::Function &To) {
  To.takeName(&From);
  To.setCallingConv(From.getCallingConv());
  To.setAttributes(Attrs);
}

bool vc::isFixedSignatureFunc(const llvm::Function &F) {
  if (F.getCallingConv() == llvm::CallingConv::SPIR_KERNEL)
    return false;
  return !F.hasLocalLinkage() || F.hasAddressTaken();
}
