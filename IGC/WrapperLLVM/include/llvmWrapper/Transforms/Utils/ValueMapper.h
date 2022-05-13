/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_UTILS_VALUEMAPPER_H
#define IGCLLVM_TRANSFORMS_UTILS_VALUEMAPPER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Transforms/Utils/ValueMapper.h"

#if LLVM_VERSION_MAJOR < 13

// LLVM 13 changed name of RF_MoveDistinctMDs to RF_ReuseAndMutateDistinctMDs
//
//     Commit link:
//     https://github.com/llvm/llvm-project/commit/fa35c1f80f0ea080a7cbc581416929b0a654f25c
#define RF_ReuseAndMutateDistinctMDs RF_MoveDistinctMDs

#endif // LLVM_VERSION_MAJOR < 13

#endif
