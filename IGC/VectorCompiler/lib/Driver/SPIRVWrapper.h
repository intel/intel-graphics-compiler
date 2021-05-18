/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

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
