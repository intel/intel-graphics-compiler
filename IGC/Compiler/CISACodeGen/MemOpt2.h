/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_MEMOPT2_H_
#define _CISA_MEMOPT2_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/PassRegistry.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

void initializeMemOpt2Pass(llvm::PassRegistry&);
llvm::FunctionPass* createMemOpt2Pass(int MLT = -1);

#endif // _CISA_MEMOPT2_H_
