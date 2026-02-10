/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_ANALYSIS_TARGETLIBRARYINFO_H
#define IGCLLVM_ANALYSIS_TARGETLIBRARYINFO_H

#include "IGC/common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "IGC/common/LLVMWarningsPop.hpp"

#define getTLI() getTLI(F)

#endif
