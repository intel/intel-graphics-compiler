/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_PreRAScheduler_H_
#define _CISA_PreRAScheduler_H_

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

void initializePreRASchedulerPass(llvm::PassRegistry&);
llvm::FunctionPass* createPreRASchedulerPass();

#endif // _CISA_PreRAScheduler_H_
