/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_LDSHRINK_H_
#define _CISA_LDSHRINK_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

void initializeLdShrinkPass(llvm::PassRegistry&);
llvm::FunctionPass* createLdShrinkPass();

#endif // _CISA_LDSHRINK_H_
