/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2023 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once
#include "Compiler/CodeGenContextWrapper.hpp"
#include "Compiler/MetaDataUtilsWrapper.h"
#include "common/MDFrameWork.h"
#include "common/LLVMWarningsPush.hpp"
#include "llvm/Config/llvm-config.h"
#include "llvm/Pass.h"
#include "llvm/IR/InstVisitor.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Analysis/LoopPass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/ConstantFolder.h"
#include "common/LLVMWarningsPop.hpp"
#include "Compiler/CodeGenPublic.h"

namespace llvm
{
    // Forward declare:
    class SampleIntrinsic;
    class SamplerLoadIntrinsic;
}

namespace IGC
{
    class CustomSafeOptPass : public llvm::FunctionPass, public llvm::InstVisitor<CustomSafeOptPass>
    {
    public:
        static char ID;

        CustomSafeOptPass();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.setPreservesCFG();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "Custom Pass Optimization";
        }

        void visitInstruction(llvm::Instruction& I);
        void visitUDiv(llvm::BinaryOperator& I);
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
        void visitLdptr(llvm::SamplerLoadIntrinsic* inst);
        void visitLdRawVec(llvm::CallInst* inst);
        void visitLoadInst(llvm::LoadInst& I);
        void dp4WithIdentityMatrix(llvm::ExtractElementInst& I);
        bool isIdentityMatrix(llvm::ExtractElementInst& I);
        void visitAnd(llvm::BinaryOperator& I);
        void visitXor(llvm::Instruction& XorInstr);
        void visitLSC2DBlockPrefetch(llvm::CallInst* I);
        void visitShuffleIndex(llvm::CallInst* I);
        void visitSelectInst(llvm::SelectInst& S);
        void mergeDotAddToDp4a(llvm::CallInst* I);

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
        void hoistDp3(llvm::BinaryOperator& I);

        template <typename MaskType> void matchReverse(llvm::BinaryOperator& I);
    private:
        bool psHasSideEffect;
        CodeGenContext* pContext = nullptr;
        IGC::ModuleMetaData* m_modMD = nullptr;
        bool lower64bto32b(llvm::BinaryOperator& AndInst);
        llvm::Value* analyzeTreeForTrunc64bto32b(const llvm::Use& OperandUse, llvm::SmallVector<llvm::BinaryOperator*, 8>& OpsToDelete);
    };

    class TrivialLocalMemoryOpsElimination : public llvm::FunctionPass, public llvm::InstVisitor<TrivialLocalMemoryOpsElimination>
    {
    public:
        static char ID;

        TrivialLocalMemoryOpsElimination();

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.addRequired<CodeGenContextWrapper>();
            AU.addRequired<MetaDataUtilsWrapper>();
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

    class GenSpecificPattern : public llvm::FunctionPass, public llvm::InstVisitor<GenSpecificPattern>
    {
    public:
        static char ID;

        GenSpecificPattern();

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

        void visitAdd(llvm::BinaryOperator& I);
        void visitAnd(llvm::BinaryOperator& I);
        void visitAShr(llvm::BinaryOperator& I);
        void visitOr(llvm::BinaryOperator& I);
        void visitShl(llvm::BinaryOperator& I);
        void visitSelectInst(llvm::SelectInst& I);
        void visitCmpInst(llvm::CmpInst& I);
        void visitZExtInst(llvm::ZExtInst& I);
        void visitCastInst(llvm::CastInst& I);
        void visitIntToPtr(llvm::IntToPtrInst& I);
        void visitMul(llvm::BinaryOperator& I);
        void visitSDiv(llvm::BinaryOperator& I);
        void visitTruncInst(llvm::TruncInst& I);
        void visitBitCastInst(llvm::BitCastInst& I);
        void visitLoadInst(llvm::LoadInst& I);
#if LLVM_VERSION_MAJOR >= 10
        void visitFNeg(llvm::UnaryOperator& I);
#endif

        void createBitcastExtractInsertPattern(llvm::BinaryOperator& I,
            llvm::Value* Op1, llvm::Value* Op2, unsigned extractNum1, unsigned extractNum2);

        void createAddcIntrinsicPattern(llvm::Instruction& I, llvm::Value* val1, llvm::Value* val2, llvm::Value* val3, llvm::Instruction& inst);
    };

    class FCmpPaternMatch : public llvm::FunctionPass, public llvm::InstVisitor<FCmpPaternMatch>
    {
    public:
        static char ID;

        FCmpPaternMatch();

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
        llvm::Module* module = nullptr;
        llvm::Constant* replaceShaderConstant(llvm::Instruction* inst);
        llvm::Constant* ConstantFoldCmpInst(llvm::CmpInst* inst);
        llvm::Constant* ConstantFoldExtractElement(llvm::ExtractElementInst* inst);
        llvm::Constant* ConstantFoldCallInstruction(llvm::CallInst* inst);
        bool simplifyAdd(llvm::BinaryOperator* BO);
        bool simplifyGEP(llvm::GetElementPtrInst* GEP);
        bool m_enableSimplifyGEP;
        const llvm::DataLayout* m_TD;
        llvm::TargetLibraryInfo* m_TLI;
    };

    llvm::FunctionPass* createVectorBitCastOptPass();
    llvm::FunctionPass* createGenStrengthReductionPass();
    llvm::FunctionPass* createNanHandlingPass();
    llvm::FunctionPass* createFlattenSmallSwitchPass();
    llvm::FunctionPass* createSplitIndirectEEtoSelPass();
    llvm::FunctionPass* createClampICBOOBAccess();
    llvm::FunctionPass* createIGCIndirectICBPropagaionPass();
    llvm::FunctionPass* createBlendToDiscardPass();
    llvm::FunctionPass* createMarkReadOnlyLoadPass();
    llvm::FunctionPass* createLogicalAndToBranchPass();
    llvm::FunctionPass* createCleanPHINodePass();
    llvm::FunctionPass* createMergeMemFromBranchOptPass();
    llvm::FunctionPass* createSinkLoadOptPass();
    llvm::FunctionPass* createInsertBranchOptPass();
} // namespace IGC
