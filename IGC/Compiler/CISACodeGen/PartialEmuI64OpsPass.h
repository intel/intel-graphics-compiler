/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef PARTIALEMUI64OPSPASS_H
#define PARTIALEMUI64OPSPASS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"

void initializePartialEmuI64OpsPass(llvm::PassRegistry&);
llvm::FunctionPass* createPartialEmuI64OpsPass();

#endif // PARTIALEMUI64OPSPASS_H
