/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef VC_UTILS_GENERAL_BIF_H
#define VC_UTILS_GENERAL_BIF_H

#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/MemoryBuffer.h>

namespace vc {

// Decodes binary module provided via \p BiFModuleBuffer. Returns obtained
// llvm::Module. If some errors occured reports fatal error.
// Note: wraps parseBitcodeFile.
std::unique_ptr<llvm::Module>
getBiFModuleOrReportError(llvm::MemoryBufferRef BiFModuleBuffer,
                          llvm::LLVMContext &Ctx);

// Same as getBiFModuleOrReportError but the decoding is lazy.
// Note: wraps getLazyBitcodeModule.
std::unique_ptr<llvm::Module>
getLazyBiFModuleOrReportError(llvm::MemoryBufferRef BiFModuleBuffer,
                              llvm::LLVMContext &Ctx);

} // namespace vc

#endif // VC_UTILS_GENERAL_BIF_H
