/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_FILESYSTEM_H
#define IGCLLVM_SUPPORT_FILESYSTEM_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/FileSystem.h"

#if LLVM_VERSION_MAJOR < 13
#define IGCLLVM_OF_None F_None
#else
#define IGCLLVM_OF_None OF_None
#endif

#endif
