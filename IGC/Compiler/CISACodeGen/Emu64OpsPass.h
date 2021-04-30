/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef EMU64OPSPASS_H
#define EMU64OPSPASS_H

#include "common/LLVMWarningsPush.hpp"
#include "llvm/Pass.h"
#include "llvm/PassRegistry.h"
#include "common/LLVMWarningsPop.hpp"

void initializeEmu64OpsPass(llvm::PassRegistry&);
llvm::FunctionPass* createEmu64OpsPass();

#endif // EMU64OPSPASS_H
