/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CISACodeGen/WIAnalysis.hpp"

namespace IGC {
// This pass performs the following optimization:
//
//  It replaces the work group reduce instruction which is only used by
//  the work item with local or global id = 0 an optimised reduce.
//  An optimized reduce uses non-zero work items less than a regular one.
//  Applies to workspace dimension equals 3 only.

class ReduceOptPass : public llvm::FunctionPass  {
public:
    static char ID;

    ReduceOptPass();

    virtual llvm::StringRef getPassName() const override {
        return "Reduce Optimisation Pass";
    }

    virtual bool runOnFunction(llvm::Function &F) override;

private:
    //This function recursively checks if the result of the reduce
    //function is only used by the work item with the local_linear_id == 0
    //or global_linear_id == 0.
    bool checkUsers(llvm::Value *MainVal, llvm::Value *Val, llvm::BasicBlock *BB);

    //Helper functions for checking the condition of using the reduce result
    bool checkSelect(llvm::Instruction *Sel);
    bool checkBranch(llvm::BasicBlock *Bb);
    bool checkCmp(llvm::Value *Val);
    bool checkGlobalId(llvm::Value *Val);
    bool checkLocalId(llvm::Value *Val);
    bool checkBuiltInName(llvm::Value *I, const std::string &Name);

    bool createReduceWI0(llvm::Instruction *ReduceInstr);

    bool Changed = false;
    llvm::Module *M = nullptr;
};

} // namespace IGC