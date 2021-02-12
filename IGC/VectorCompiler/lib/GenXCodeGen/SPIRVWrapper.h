/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/
//
// Currently VectorCompiler uses a special type of LLVM-SPIRV-Translator
// in a form of shared library called "SPIRVDLL"
// It is expected to move from this solution in favour of original Khronos
// LLVMSPIRVLib library.
// This file was created for the purpose of smooth transit between these two
// library versions.
//
//===----------------------------------------------------------------------===//

#ifndef SPIRV_WRAPPER_H
#define SPIRV_WRAPPER_H

#include "llvm/ADT/ArrayRef.h"
#include "llvm/Support/Error.h"

namespace vc {
llvm::Expected<std::vector<char>>
translateSPIRVToIR(llvm::ArrayRef<char> Input,
                   llvm::ArrayRef<uint32_t> SpecConstIds,
                   llvm::ArrayRef<uint64_t> SpecConstValues);
}

#endif // SPIRV_WRAPPER_H
