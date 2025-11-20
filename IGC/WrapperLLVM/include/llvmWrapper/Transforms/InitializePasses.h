/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H
#define IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H

namespace llvm {
class PassRegistry;
}

void initializeADCELegacyPassWrapperPass(llvm::PassRegistry &);

#endif // IGCLLVM_TRANSFORMS_INITIALIZE_PASSES_H
