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
#include <llvm/Pass.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    class CShader;

    /// @brief sort basic blocks into topological order
    /// Arbitrary reverse postorder is not sufficient.
    /// Whenever it is possible, we want to layout blocks in such way 
    /// that the vIsa Jitter can recognize the control-flow structures
    class Layout : public llvm::FunctionPass
    {
    public:
        static char ID;
        Layout();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override;

        /// @brief Provides name of pass
        virtual llvm::StringRef getPassName() const override {
            return "Layout";
        }

        virtual bool runOnFunction(llvm::Function& func) override;

    private:
        llvm::BasicBlock* getLastReturnBlock(llvm::Function& Func);
        void LayoutBlocks(llvm::Function& func, llvm::LoopInfo& LI);
        void LayoutBlocks(llvm::Function& func);
        llvm::BasicBlock* selectSucc(
            llvm::BasicBlock* CurrBlk,
            bool SelectNoInstBlk,
            const llvm::LoopInfo& LI,
            const std::set<llvm::BasicBlock*>& VisitSet);


        llvm::PostDominatorTree* m_PDT;
    };

}
