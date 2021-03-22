/*========================== begin_copyright_notice ============================

Copyright (c) 2021 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
IN THE SOFTWARE.

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
