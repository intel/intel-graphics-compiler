/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenContextWrapper.hpp"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/LoopInfo.h>
#include <llvm/Analysis/LoopPass.h>
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

        CheckInstrTypes();
        CheckInstrTypes(bool afterOpts, bool metrics);

        virtual bool runOnFunction(llvm::Function& F) override;
        bool doFinalization(llvm::Module&) override;

        void checkGlobalLocal(llvm::Instruction& I);

        virtual llvm::StringRef getPassName() const override
        {
            return "CheckInstrTypes";
        }

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
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
        IGC::SInstrTypes g_InstrTypes = {};
        bool g_AfterOpts = false, g_metrics = false;
        CodeGenContext* context = nullptr;

        void print(llvm::raw_ostream& OS) const;
    };

    class InstrStatistic : public llvm::FunctionPass, public llvm::InstVisitor<InstrStatistic>
    {
    public:
        static char ID;
        InstrStatistic() : FunctionPass(ID), m_ctx(nullptr), m_type(InstrStatTypes(0)),
            m_stage(InstrStatStage::BEGIN), m_threshold(0), m_LI(nullptr)
        {
        };
        InstrStatistic(CodeGenContext* ctx, InstrStatTypes type, InstrStatStage stage, int threshold);

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "InstrStatistic";
        }

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::LoopInfoWrapperPass>();
            AU.setPreservesAll();
        }

    private:
        CodeGenContext* m_ctx;
        IGC::InstrStatTypes m_type;
        InstrStatStage m_stage;
        int m_threshold;
        llvm::LoopInfo* m_LI;

        bool parseLoops();
        bool parseLoop(llvm::Loop* loop);

        void print(llvm::raw_ostream& OS) const;
    };

} // namespace IGC
