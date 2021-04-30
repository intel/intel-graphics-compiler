/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "common/Stats.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Module.h>
#include "common/LLVMWarningsPop.hpp"

class PassTimer : public llvm::ModulePass
{
public:
    PassTimer(IGC::CodeGenContext* ctx, COMPILE_TIME_INTERVALS index, bool isStart) : llvm::ModulePass(ID)
    {
        m_context = ctx;
        m_index = index;
        m_isStart = isStart;
    }
    virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
    {
        AU.setPreservesAll();
    }

    virtual bool runOnModule(llvm::Module& M) override;

    virtual llvm::StringRef getPassName() const override
    {
        return "passTimer";
    }

private:
    IGC::CodeGenContext* m_context;
    static char ID;
    COMPILE_TIME_INTERVALS m_index;
    bool m_isStart;
};

