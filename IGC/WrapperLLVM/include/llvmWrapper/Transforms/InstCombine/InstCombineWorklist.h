/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_INSTCOMBINEWORKLIST_H
#define IGCLLVM_TRANSFORMS_UTILS_INSTCOMBINEWORKLIST_H

#include "llvm/Config/llvm-config.h"

#if LLVM_VERSION_MAJOR <= 13
#include "llvm/Transforms/InstCombine/InstCombineWorklist.h"
#else
#include "llvm/Transforms/Utils/InstructionWorklist.h"
using InstCombineWorklist = llvm::InstructionWorklist;
#endif
#endif
