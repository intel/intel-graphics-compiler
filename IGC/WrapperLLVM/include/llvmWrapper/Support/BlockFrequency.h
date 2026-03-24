/*========================== begin_copyright_notice ============================

Copyright (C) 2026 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_BLOCKFREQUENCY_H
#define IGCLLVM_SUPPORT_BLOCKFREQUENCY_H

#include "llvm/Support/BlockFrequency.h"

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR >= 22
inline uint64_t getFrequency(llvm::BlockFrequency BF) { return BF.getFrequency(); }
#else
inline uint64_t getFrequency(uint64_t Freq) { return Freq; }
#endif
} // namespace IGCLLVM

#endif // IGCLLVM_SUPPORT_BLOCKFREQUENCY_H
