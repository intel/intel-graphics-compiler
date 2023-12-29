/*========================== begin_copyright_notice ============================

Copyright (C) 2019-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/DependenceAnalysis.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/ADT/SmallSet.h>
#include <llvm/Pass.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
namespace IGC {
    class SampleMultiversioning : public llvm::FunctionPass,
        public llvm::InstVisitor<SampleMultiversioning> {
    public:
        static char ID;
        CodeGenContext* pContext;
        DominatorTree* DT = nullptr;

        SampleMultiversioning(CodeGenContext* pContext);
        SampleMultiversioning();
        ~SampleMultiversioning() {}

        virtual llvm::StringRef getPassName() const override {
            return "Sample Multiversioning";
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
        }
    private:
        bool isOnlyExtractedAfterSample(Value* SampleInst, SmallVector<Instruction*, 4> & ExtrVals);
        bool isOnlyMultiplied(Instruction* Sample, Instruction* Val, SmallSet<Instruction*, 4> & MulVals);
        Instruction* getPureFunction(Value* Val);
        bool isOnlyMultipliedAfterSample(Instruction* Val, SmallSet<Instruction*, 4> & MulVals);
    };
} // namespace IGC
