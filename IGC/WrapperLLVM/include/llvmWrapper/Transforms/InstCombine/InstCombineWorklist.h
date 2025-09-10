/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_INSTCOMBINEWORKLIST_H
#define IGCLLVM_TRANSFORMS_UTILS_INSTCOMBINEWORKLIST_H

#include "llvm/Config/llvm-config.h"

#define DEBUG_TYPE "instcombine"
#include "llvm/Transforms/Utils/InstructionWorklist.h"
#undef DEBUG_TYPE
using InstCombineWorklist = llvm::InstructionWorklist;
#endif
