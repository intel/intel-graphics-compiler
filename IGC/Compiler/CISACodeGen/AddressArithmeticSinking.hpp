/*========================== begin_copyright_notice ============================

Copyright (C) 2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

/*========================== begin_copyright_notice ============================

This file is distributed under the University of Illinois Open Source License.
See LICENSE.TXT for details.

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Analysis/PostDominators.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"


namespace IGC {

class AddressArithmeticSinking : public llvm::FunctionPass {

private:
    llvm::DominatorTree* DT;

public:
    static char ID;
    // SinkingDepth argument to set m_SinkingDepth member
    // Set it to 4 as default because from experiment this value provide
    // good decreasing register pressure and does not greatly increase dynamic
    // instruction counter
    AddressArithmeticSinking(unsigned SinkingDepth = 4);
    virtual bool runOnFunction(llvm::Function& F) override;

    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
        AU.setPreservesCFG();
        AU.addRequired<llvm::DominatorTreeWrapperPass>();
        AU.addPreserved<llvm::DominatorTreeWrapperPass>();
    }

private:
    // Parameter to limit the number of sinked instruction in address arithmetic chain
    unsigned m_SinkingDepth;

    bool sink(llvm::Instruction* I);
    bool ProcessBB(llvm::BasicBlock* BB);

    llvm::BasicBlock* FindSinkTarget(llvm::Instruction* I);
};
void initializeAddressArithmeticSinkingPass(llvm::PassRegistry&);

} // namespace IGC

