/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

#include "PrintCodeGenContextPass.h"

#include "Compiler/CodeGenPublic.h"

namespace IGC {
char PrintCodeGenContextPass::ID = 0;

PrintCodeGenContextPass::PrintCodeGenContextPass(llvm::raw_ostream &Stream)
    : Stream(Stream), ModulePass(ID) {}

bool PrintCodeGenContextPass::runOnModule(llvm::Module &M) {
    getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->print(Stream);
    return false;
}
} // namespace IGC
