/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_ADVCODEMOTION_H_
#define _CISA_ADVCODEMOTION_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    void initializeAdvCodeMotionPass(llvm::PassRegistry&);
    llvm::FunctionPass* createAdvCodeMotionPass(unsigned Control);
} // End namespace IGC

void initializeMadLoopSlicePass(llvm::PassRegistry&);
llvm::FunctionPass* createMadLoopSlicePass();

#endif // _CISA_ADVCODEMOTION_H_
