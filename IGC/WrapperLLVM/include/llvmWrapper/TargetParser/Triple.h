/*========================== begin_copyright_notice ============================

Copyright (C) 2025 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/
#ifndef IGCLLVM_TARGETPARSER_TRIPLE_H
#define IGCLLVM_TARGETPARSER_TRIPLE_H

#if LLVM_VERSION_MAJOR > 16 && !defined(IGC_LLVM_TRUNK_REVISION)
#include <llvm/TargetParser/Triple.h>
#else
#include <llvm/ADT/Triple.h>
#endif

#endif // IGCLLVM_TARGETPARSER_TRIPLE_H
