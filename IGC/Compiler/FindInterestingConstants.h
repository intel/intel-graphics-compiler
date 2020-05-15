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
#include "llvm/Analysis/LoopInfo.h"
namespace IGC
{
    struct InstructionStats
    {
        uint32_t instCount = 0;
        uint32_t branchCount = 0;
        uint32_t loopCount = 0;
        uint32_t samplerCount = 0;
        uint32_t extendedMath = 0;
        uint32_t selectCount = 0;
        uint32_t weight = 0;
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
        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<llvm::LoopInfoWrapperPass>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;
        bool doFinalization(llvm::Module&) override;
        void visitLoadInst(llvm::LoadInst& I);

    private:
        CodeGenContext* m_context;
        llvm::LoopInfo* m_LI;
        unsigned int m_instCount;
        unsigned int m_constFoldBranch;
        unsigned int m_constFoldLoopBranch;
        unsigned int m_samplerCount;
        unsigned int m_extendedMath;
        unsigned int m_selectCount;
        unsigned int m_branchsize;
        unsigned int m_loopSize;
        std::unordered_map<unsigned int, std::vector<SConstantAddrValue>> m_InterestingConstants;
        const llvm::DataLayout* m_DL;
        std::unordered_set<llvm::Instruction*> visitedForFolding;
        // Helper functions
        bool isReverseOpInstPair(llvm::Intrinsic::ID intr1, llvm::Intrinsic::ID intr2);
        void UpdateInstCount(llvm::Instruction* inst);
        bool getConstantAddress(llvm::LoadInst& I, unsigned& bufIdOrGRFOffset, int& eltId, int& size_in_bytes);
        bool FoldsToConst(llvm::Instruction* inst, llvm::Instruction* use, bool& propagate);
        bool FoldsToZero(llvm::Instruction* inst, llvm::Instruction* use);
        bool FoldsToSource(llvm::Instruction* inst, llvm::Instruction* use);
        void FoldsToConstPropagate(llvm::Instruction* I);
        void FoldsToZeroPropagate(llvm::Instruction* I);
        void FoldsToSourcePropagate(llvm::Instruction* I);
        bool allUsersVisitedForFolding(llvm::Instruction* binOperand, llvm::Instruction* binInst);
        void CheckIfSampleBecomesDeadCode(llvm::Instruction* inst, llvm::Instruction* use);
        unsigned int BranchSize(llvm::Instruction* I, llvm::BranchInst* Br, bool& isLoop);
        void ResetStatCounters();
        void addInterestingConstant(llvm::Type* loadTy, unsigned bufIdOrGRFOffset, unsigned eltId, int size_in_bytes, bool anyValue, uint32_t constValue, InstructionStats stats);
        template<typename ContextT>
        void copyInterestingConstants(ContextT* pShaderCtx);
    };
}


