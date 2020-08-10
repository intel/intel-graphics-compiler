/*===================== begin_copyright_notice ==================================

Copyright (c) 2017 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be included
in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


======================= end_copyright_notice ==================================*/

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
        DominatorTree* DT;

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
