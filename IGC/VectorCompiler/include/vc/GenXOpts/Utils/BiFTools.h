/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

#ifndef VC_GENX_OPTS_UTILS_BIF_TOOLS_H
#define VC_GENX_OPTS_UTILS_BIF_TOOLS_H

namespace llvm {

// Decodes binary module provided via \p BiFModuleBuffer. Returns obtained
// llvm::Module. If some errors occured reports fatal error.
// Note: wraps parseBitcodeFile.
std::unique_ptr<Module>
getBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer, LLVMContext &Ctx);

// Same as getBiFModuleOrReportError but the decoding is lazy.
// Note: wraps getLazyBitcodeModule.
std::unique_ptr<Module>
getLazyBiFModuleOrReportError(MemoryBufferRef BiFModuleBuffer,
                              LLVMContext &Ctx);

} // namespace llvm

#endif // VC_GENX_OPTS_UTILS_BIF_TOOLS_H
