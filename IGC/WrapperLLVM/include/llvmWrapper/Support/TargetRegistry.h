/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_TARGETREGISTRY_H
#define IGCLLVM_SUPPORT_TARGETREGISTRY_H

#include "llvm/Config/llvm-config.h"

#if LLVM_VERSION_MAJOR < 14
#include "llvm/Support/TargetRegistry.h"
#else
#include "llvm/MC/TargetRegistry.h"
#endif

#endif
