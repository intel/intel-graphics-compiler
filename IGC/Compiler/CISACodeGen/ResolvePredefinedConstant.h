/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_RESOLVEPREDEFINEDCONSTANT_H_
#define _CISA_RESOLVEPREDEFINEDCONSTANT_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
    llvm::ModulePass* createResolvePredefinedConstantPass();
    void initializePredefinedConstantResolvingPass(llvm::PassRegistry&);
} // End namespace IGC

#endif // _CISA_RESOLVEPREDEFINEDCONSTANT_H_
