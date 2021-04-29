/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

// This file implements conversion of SPIR-V binary to LLVM IR.

#ifndef SPIRVCONSUM_HPP_
#define SPIRVCONSUM_HPP_

#include "llvm/IR/Module.h"

#include <unordered_map>

namespace igc_spv{
// Loads SPIRV from istream and translate to LLVM module.
// Returns true if succeeds.
bool ReadSPIRV(llvm::LLVMContext &C, std::istream &IS, llvm::Module *&M,
    std::string &ErrMsg,
    std::unordered_map<uint32_t, uint64_t> *specConstants);

}
#endif
