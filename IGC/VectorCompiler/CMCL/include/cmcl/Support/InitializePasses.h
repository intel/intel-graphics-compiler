/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_CM_CL_SUPPORT_INITIALIZE_PASSES_H
#define VC_CM_CL_SUPPORT_INITIALIZE_PASSES_H

#include <llvm/PassRegistry.h>

namespace cmcl {
void initializeLLVMPasses(llvm::PassRegistry &);
}

extern "C" void CMCLInitializeLLVMPasses();

#endif // VC_CM_CL_SUPPORT_INITIALIZE_PASSES_H
