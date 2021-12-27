/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//
// Legacy SPIRVDLL-like interface for translation of SPIRV to LLVM IR.
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
