/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_HOST_H
#define IGCLLVM_SUPPORT_HOST_H

#if LLVM_VERSION_MAJOR < 22
#include <llvm/Support/Host.h>
#else
#include <llvm/TargetParser/Host.h>
#endif

#endif // IGCLLVM_SUPPORT_HOST_H
