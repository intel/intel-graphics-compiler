/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef LEGALIZER_TYPELEGALIZERPASS_H
#define LEGALIZER_TYPELEGALIZERPASS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"

void initializeTypeLegalizerPass(llvm::PassRegistry&);
llvm::FunctionPass* createTypeLegalizerPass();

#endif // LEGALIZER_TYPELEGALIZERPASS_H
