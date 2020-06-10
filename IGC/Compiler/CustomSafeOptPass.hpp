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
#include "common/MDFrameWork.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/LoopPass.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/ConstantFolder.h>
#include "common/LLVMWarningsPop.hpp"

namespace llvm
{
    // Forward declare:
    class SampleIntrinsic;
}

namespace IGC
{
    class CustomSafeOptPass : public llvm::FunctionPass, public llvm::InstVisitor<CustomSafeOptPass>
    {
    public:
        static char ID;

        CustomSafeOptPass();

        ~CustomSafeOptPass() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "Custom Pass Optimization";
        }

        void visitInstruction(llvm::Instruction& I);
        void visitAllocaInst(llvm::AllocaInst& I);
        void visitCallInst(llvm::CallInst& C);
        void removeHftoFCast(llvm::Instruction& I);
        void visitBinaryOperator(llvm::BinaryOperator& I);
        bool isEmulatedAdd(llvm::BinaryOperator& I);
        void visitBfi(llvm::CallInst* inst);
        void visitf32tof16(llvm::CallInst* inst);
        void visitSampleBptr(llvm::SampleIntrinsic* inst);
        void visitMulH(llvm::CallInst* inst, bool isSigned);
        void visitFPToUIInst(llvm::FPToUIInst& FPUII);
        void visitFPTruncInst(llvm::FPTruncInst& I);
        void visitExtractElementInst(llvm::ExtractElementInst& I);
        void visitLdptr(llvm::CallInst* inst);
        void visitLdRawVec(llvm::CallInst* inst);
        void visitLoadInst(llvm::LoadInst& I);

        //
        // IEEE Floating point arithmetic is not associative.  Any pattern
        // match that changes the order or paramters is unsafe.
        //

        //
        // Removing sources is also unsafe.
        //  X * 1 => X     : Unsafe
        //  X + 0 => X     : Unsafe
        //  X - X => X     : Unsafe
        //

        // When in doubt assume a floating point optimization is unsafe!

        void visitBinaryOperatorTwoConstants(llvm::BinaryOperator& I);
        void visitBinaryOperatorPropNegate(llvm::BinaryOperator& I);
        void visitBitCast(llvm::BitCastInst& BC);

        void matchDp4a(llvm::BinaryOperator& I);

    private:
        bool psHasSideEffect;
    };

#if LLVM_VERSION_MAJOR >= 7
    class TrivialLocalMemoryOpsElimination : public llvm::FunctionPass, public llvm::InstVisitor<TrivialLocalMemoryOpsElimination>
    {
    public:
        static char ID;

        TrivialLocalMemoryOpsElimination();

        ~TrivialLocalMemoryOpsElimination() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "TrivialLocalMemoryOpsElimination";
        }

        void visitLoadInst(llvm::LoadInst& I);
        void visitStoreInst(llvm::StoreInst& I);
        void visitCallInst(llvm::CallInst& I);
        bool isLocalBarrier(llvm::CallInst& I);
        void findNextThreadGroupBarrierInst(llvm::Instruction& I);
        void anyCallInstUseLocalMemory(llvm::CallInst& I);

    private:
        llvm::SmallVector<llvm::LoadInst*, 16> m_LocalLoadsToRemove;
        llvm::SmallVector<llvm::StoreInst*, 16> m_LocalStoresToRemove;
        llvm::SmallVector<llvm::CallInst*, 16> m_LocalFencesBariersToRemove;

        bool abortPass = false;
        const std::vector<bool> m_argumentsOfLocalMemoryBarrier{ true, false, false, false, false, false, true };
    };
#endif

    class GenSpecificPattern : public llvm::FunctionPass, public llvm::InstVisitor<GenSpecificPattern>
    {
    public:
        static char ID;

        GenSpecificPattern();

        ~GenSpecificPattern() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "GenSpecificPattern";
        }

        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitSelectInst(llvm::SelectInst& I);
        void visitCmpInst(llvm::CmpInst& I);
        void visitZExtInst(llvm::ZExtInst& I);
        void visitCastInst(llvm::CastInst& I);
        void visitIntToPtr(llvm::IntToPtrInst& I);
        void visitSDiv(llvm::BinaryOperator& I);
        void visitTruncInst(llvm::TruncInst& I);
        void visitBitCastInst(llvm::BitCastInst& I);
#if LLVM_VERSION_MAJOR >= 10
        void visitFNeg(llvm::UnaryOperator& I);
#endif

        template <typename MaskType> void matchReverse(llvm::BinaryOperator& I);
        void createBitcastExtractInsertPattern(llvm::BinaryOperator& I,
            llvm::Value* Op1, llvm::Value* Op2, unsigned extractNum1, unsigned extractNum2);
    };

    class FCmpPaternMatch : public llvm::FunctionPass, public llvm::InstVisitor<FCmpPaternMatch>
    {
    public:
        static char ID;

        FCmpPaternMatch();

        ~FCmpPaternMatch() {}

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "FCmpPaternMatch";
        }

        void visitSelectInst(llvm::SelectInst& I);
    };

    class IGCConstProp : public llvm::FunctionPass
    {
    public:
        static char ID;

        IGCConstProp(bool enableSimplifyGEP = false);

        ~IGCConstProp() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<llvm::TargetLibraryInfoWrapperPass>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            // specialized const-prop with shader-const replacement
            return "const-prop with shader-const replacement";
        }

    private:
        llvm::Module* module;
        llvm::Constant* replaceShaderConstant(llvm::LoadInst* inst);
        llvm::Constant* ConstantFoldCmpInst(llvm::CmpInst* inst);
        llvm::Constant* ConstantFoldExtractElement(llvm::ExtractElementInst* inst);
        llvm::Constant* ConstantFoldCallInstruction(llvm::CallInst* inst);
        bool simplifyAdd(llvm::BinaryOperator* BO);
        bool simplifyGEP(llvm::GetElementPtrInst* GEP);
        bool m_enableMathConstProp;
        bool m_enableSimplifyGEP;
        const llvm::DataLayout* m_TD;
        llvm::TargetLibraryInfo* m_TLI;
    };

    llvm::FunctionPass* createGenStrengthReductionPass();
    llvm::FunctionPass* createNanHandlingPass();
    llvm::FunctionPass* createFlattenSmallSwitchPass();
    llvm::FunctionPass* createIGCIndirectICBPropagaionPass();
    llvm::FunctionPass* createBlendToDiscardPass();
    llvm::FunctionPass* createMarkReadOnlyLoadPass();
    llvm::FunctionPass* createLogicalAndToBranchPass();

} // namespace IGC
