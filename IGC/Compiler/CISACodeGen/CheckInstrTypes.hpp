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
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/IGCPassSupport.h"
#include "Compiler/CodeGenPublic.h"

namespace IGC
{
    // Forward declaration
    struct SInstrTypes;

    class CheckInstrTypes : public llvm::FunctionPass, public llvm::InstVisitor<CheckInstrTypes>
    {
        llvm::LoopInfo* LI;

    public:
        static char ID;
        CheckInstrTypes() : FunctionPass(ID), g_InstrTypes(nullptr)
        {
        };
        CheckInstrTypes(IGC::SInstrTypes* instrList);

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "CheckInstrTypes";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.setPreservesAll();
        }

        void visitInstruction(llvm::Instruction& I);

        void visitCallInst(llvm::CallInst& C);
        void visitBranchInst(llvm::BranchInst& I);
        void visitSwitchInst(llvm::SwitchInst& I);
        void visitIndirectBrInst(llvm::IndirectBrInst& I);
        void visitICmpInst(llvm::ICmpInst& I);
        void visitFCmpInst(llvm::FCmpInst& I);
        void visitAllocaInst(llvm::AllocaInst& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitGetElementPtrInst(llvm::GetElementPtrInst& I);
        void visitPHINode(llvm::PHINode& PN);
        void visitSelectInst(llvm::SelectInst& I);
        void SetLoopFlags(llvm::Function& F);

    private:
        IGC::SInstrTypes* g_InstrTypes;

    };

    class InstrStatitic : public llvm::FunctionPass, public llvm::InstVisitor<InstrStatitic>
    {
    public:
        static char ID;
        InstrStatitic() : FunctionPass(ID), m_ctx(nullptr), m_type(InstrStatTypes(0)), m_stage(InstrStatStage::BEGIN), m_threshold(0)
        {
        };
        InstrStatitic(CodeGenContext* ctx, InstrStatTypes type, InstrStatStage stage, int threshold);

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "InstrStatitic";
        }

        void visitInstruction(llvm::Instruction& I);
        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);

    private:
        CodeGenContext* m_ctx;
        IGC::InstrStatTypes m_type;
        InstrStatStage m_stage;
        int m_threshold;
    };

} // namespace IGC