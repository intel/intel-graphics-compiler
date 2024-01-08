/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Instructions.h"
#include "common/LLVMWarningsPop.hpp"

#include "PrintCodeGenContextPass.h"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"

using namespace llvm;
namespace IGC {

#define PASS_FLAG "print-codegencontext"
#define PASS_DESCRIPTION "Print CodeGenContext"
#define PASS_CFG_ONLY true
#define PASS_ANALYSIS true
IGC_INITIALIZE_PASS_BEGIN(PrintCodeGenContextPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(PrintCodeGenContextPass, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char PrintCodeGenContextPass::ID = 0;

PrintCodeGenContextPass::PrintCodeGenContextPass()
    : Stream(llvm::errs()), ModulePass(ID) {}

PrintCodeGenContextPass::PrintCodeGenContextPass(llvm::raw_ostream &Stream)
    : Stream(Stream), ModulePass(ID) {}

bool PrintCodeGenContextPass::runOnModule(llvm::Module &M) {
    getAnalysis<CodeGenContextWrapper>().getCodeGenContext()->print(Stream);
    return false;
}
} // namespace IGC
