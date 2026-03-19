/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_TRANSFORMS_VECTORIZE_LEGACY_H
#define IGCLLVM_TRANSFORMS_VECTORIZE_LEGACY_H

#if LLVM_VERSION_MAJOR < 22
#include "llvm/Transforms/Vectorize.h"
#else
#include "llvm/Transforms/Vectorize/LoadStoreVectorizer.h"
#endif

#endif // IGCLLVM_TRANSFORMS_VECTORIZE_LEGACY_H
