/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef _CISA_TYPEDEMOTE_H_
#define _CISA_TYPEDEMOTE_H_

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

namespace IGC {
    llvm::FunctionPass* createTypeDemotePass();
    void initializeTypeDemotePass(llvm::PassRegistry&);
} // End namespace IGC

#endif // _CISA_TYPEDEMOTE_H_
