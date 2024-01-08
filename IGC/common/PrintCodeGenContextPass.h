/*========================== begin_copyright_notice ============================

Copyright (C) 2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"

#include "common/LLVMWarningsPush.hpp"
#include "llvm/IR/Function.h"
#include "llvm/IR/Metadata.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Value.h"
#include "llvm/Pass.h"
#include "common/LLVMWarningsPop.hpp"

namespace IGC {
class PrintCodeGenContextPass : public llvm::ModulePass {
public:
    static char ID;

    PrintCodeGenContextPass();
    PrintCodeGenContextPass(llvm::raw_ostream &Stream);
    bool runOnModule(llvm::Module &M) override;

    void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.addRequired<CodeGenContextWrapper>();
    }

private:
    llvm::raw_ostream& Stream;
};
void initializePrintCodeGenContextPassPass(llvm::PassRegistry&);
} // namespace IGC
