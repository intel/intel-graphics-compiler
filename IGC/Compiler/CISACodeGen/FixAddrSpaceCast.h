/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_FIXADDRSPACECAST_H_
#define _CISA_FIXADDRSPACECAST_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    llvm::FunctionPass* createFixAddrSpaceCastPass();
    void initializeAddrSpaceCastFixingPass(llvm::PassRegistry&);
} // End namespace IGC

#endif // _CISA_FIXADDRSPACECAST_H_
