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

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Dominators.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    struct ConstInstrCBLoads
    {
        bool isConst;
        std::map<llvm::Value*, IGC::ConstantAddress> CBloads;
    };

    class FindInterestingConstants : public llvm::FunctionPass, public llvm::InstVisitor<FindInterestingConstants>
    {
    public:
        static char ID;

        FindInterestingConstants();
        ~FindInterestingConstants() {}

        virtual llvm::StringRef getPassName() const override
        {
            return "Find Interesting Constants";
        }
        virtual void getAnalysisUsage(llvm::AnalysisUsage &AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function &F) override;
        bool doFinalization(llvm::Module &) override;
        void visitLoadInst(llvm::LoadInst &I);

    private:
        unsigned int m_foldsToZero;
        unsigned int m_foldsToConst;
        unsigned int m_foldsToSource;
        bool m_constFoldBranch;
        std::vector<SConstantAddrValue> m_InterestingConstants;

        // Helper functions
        bool getConstantAddress(llvm::LoadInst &I, unsigned &bufId, unsigned &eltId, int &size_in_bytes);
        bool FoldsToConst(llvm::Instruction* inst, llvm::Instruction* use, bool &propagate);
        bool FoldsToZero(llvm::Instruction* inst, llvm::Instruction* use);
        bool FoldsToSource(llvm::Instruction* inst, llvm::Instruction* use);
        void FoldsToConstPropagate(llvm::Instruction* I);
        void FoldsToZeroPropagate(llvm::Instruction* I);
        void FoldsToSourcePropagate(llvm::Instruction* I);
        void addInterestingConstant(CodeGenContext* ctx, unsigned bufId, unsigned eltId, int size_in_bytes, bool anyValue, uint32_t value);
        template<typename ContextT>
        void copyInterestingConstants(ContextT* pShaderCtx);
    };
}


