/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_IR_COMDAT_H
#define IGCLLVM_IR_COMDAT_H

#include "llvm/Config/llvm-config.h"
#include "llvm/IR/Comdat.h"

#if LLVM_VERSION_MAJOR < 13
#define IGCLLVM_NoDuplicates NoDeduplicate
#else
#define IGCLLVM_NoDuplicates NoDuplicates
#endif

#endif
