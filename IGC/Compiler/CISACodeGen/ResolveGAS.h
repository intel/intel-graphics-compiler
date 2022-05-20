/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_RESOLVEGAS_H_
#define _CISA_RESOLVEGAS_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    llvm::FunctionPass* createResolveGASPass();
    void initializeGASResolvingPass(llvm::PassRegistry&);

    llvm::ModulePass* createGASRetValuePropagatorPass();
    void initializeGASRetValuePropagatorPass(llvm::PassRegistry&);

    llvm::ModulePass* createLowerGPCallArg();
    void initializeLowerGPCallArgPass(llvm::PassRegistry&);
} // End namespace IGC

#endif // _CISA_RESOLVEGAS_H_
