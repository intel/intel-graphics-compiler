/*========================== begin_copyright_notice ============================

Copyright (C) 2018-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef IGCLLVM_SUPPORT_MEMORYBUFFER_H
#define IGCLLVM_SUPPORT_MEMORYBUFFER_H

#include "llvm/Config/llvm-config.h"
#include "llvm/Support/MemoryBuffer.h"

namespace IGCLLVM {
#if LLVM_VERSION_MAJOR < 9
    // There's only MemoryBufferRef::MemoryBufferRef(MemoryBuffer&) prior to LLVM-9.
    // Though inconstancy is not required.
    static inline llvm::MemoryBufferRef makeMemoryBufferRef(const llvm::MemoryBuffer &Buffer) {
        return llvm::MemoryBufferRef{Buffer.getBuffer(), Buffer.getBufferIdentifier()};
    }
#else
    static inline llvm::MemoryBufferRef makeMemoryBufferRef(const llvm::MemoryBuffer &Buffer) {
        return llvm::MemoryBufferRef{Buffer};
    }
#endif
} // namespace IGCLLVM
#endif
