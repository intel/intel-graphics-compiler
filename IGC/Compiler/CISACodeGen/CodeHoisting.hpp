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
#include "common/LLVMWarningsPush.hpp"
#include <llvm/ADT/MapVector.h>
#include <llvm/Analysis/PostDominators.h>
#include "llvm/Analysis/LoopInfo.h"
#include "llvm/Analysis/PostDominators.h"
#include "llvm/IR/Dominators.h"
#include "common/LLVMWarningsPop.hpp"


namespace IGC {

    class CodeHoisting : public llvm::FunctionPass {
        llvm::DominatorTree* DT;
        llvm::PostDominatorTree* PDT;
        llvm::LoopInfo* LI;

    public:
        static char ID; // Pass identification

        CodeHoisting();

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override {
            AU.setPreservesCFG();
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addPreserved<llvm::DominatorTreeWrapperPass>();
            AU.addPreserved<llvm::PostDominatorTreeWrapperPass>();
            AU.addPreserved<llvm::LoopInfoWrapperPass>();
        }

    private:
        void gatherLastURBReadInEachBB(llvm::Function& F);
        llvm::Instruction* searchBackForAliasedURBRead(
            llvm::Instruction* urbWrite);

        void hoistURBWriteInBB(llvm::BasicBlock& BB);

        /// local processing
        bool isSafeToHoistURBWriteInstruction(
            llvm::Instruction* inst,
            llvm::Instruction*& tgtInst);

        /// data members for local-hoisting
        llvm::MapVector<llvm::Instruction*, llvm::Instruction*> instMovDataMap;
        llvm::DenseMap<llvm::BasicBlock*, llvm::Instruction*> basicBlockReadInstructionMap;
    };

}

