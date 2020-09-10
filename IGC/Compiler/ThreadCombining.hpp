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
#include <llvm/ADT/SetVector.h>
#include <llvm/Pass.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Dominators.h>
#include <llvm/Analysis/PostDominators.h>
#include "common/LLVMWarningsPop.hpp"

#include "Compiler/CodeGenPublic.h"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"

void initializeThreadCombiningPass(llvm::PassRegistry&);

namespace IGC
{
    enum dim
    {
        ThreadGroupSize_X,
        ThreadGroupSize_Y,
        ThreadGroupSize_Z
    };

    class ThreadCombining : public llvm::ModulePass
    {
    public:
        ThreadCombining()
            : ModulePass(ID)
            , m_kernel(nullptr)
            , m_SLMUsed(false)
        {
            initializeThreadCombiningPass(*llvm::PassRegistry::getPassRegistry());
        }

        ~ThreadCombining() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "ThreadCombining";
        }

        bool runOnModule(llvm::Module& M) override;

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::DominatorTreeWrapperPass>();
            AU.addRequired<llvm::PostDominatorTreeWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
        }

        llvm::Function* m_kernel;
        bool m_SLMUsed;
        std::vector<llvm::Instruction*> m_barriers;
        llvm::SetVector<llvm::Instruction*> m_aliveAcrossBarrier;
        std::set<llvm::Instruction*> m_instructionsToMove;
        std::map<llvm::Instruction*, std::set<llvm::Instruction*>> m_LiveRegistersPerBarrier;
        static char ID;

        bool isBarrier(llvm::Instruction& I) const;
        bool isSLMUsed(llvm::Instruction* I) const;
        unsigned int GetthreadGroupSize(llvm::Module& M, dim dimension);
        void SetthreadGroupSize(llvm::Module& M, llvm::Constant* size, dim dimension);
        void remapThreads(llvm::Module& M,
            unsigned int newSizeX,
            unsigned int newSizeY,
            unsigned int threadGroupSize_X,
            unsigned int threadGroupSize_Y,
            llvm::IRBuilder<>& builder);

        void CreateLoopKernel(
            llvm::Module& M,
            unsigned int newSizeX,
            unsigned int newSizeY,
            unsigned int threadGroupSize_X,
            unsigned int threadGroupSize_Y,
            llvm::Function* newFunc,
            llvm::IRBuilder<>& builder);

        void CreateNewKernel(llvm::Module& M,
            llvm::IRBuilder<>& builder,
            llvm::Function* newFunc);

        bool canDoOptimization(llvm::Function* m_kernel, llvm::Module& M);
        void PreAnalysis(llvm::Function* m_kernel, llvm::Module& M, std::vector<llvm::Instruction*>& barriers);
        void FindRegistersAliveAcrossBarriers(llvm::Function* m_kernel, llvm::Module& M);
    };
}