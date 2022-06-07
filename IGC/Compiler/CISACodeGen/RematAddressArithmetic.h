/*========================== begin_copyright_notice ============================

Copyright (C) 2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_REMATADDRESSARITHMETIC_H_
#define _CISA_REMATADDRESSARITHMETIC_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    llvm::FunctionPass* createRematAddressArithmeticPass();
    void initializeRematAddressArithmeticPass(llvm::PassRegistry&);
} // End namespace IGC

#endif // _CISA_REMATADDRESSARITHMETIC_H_
