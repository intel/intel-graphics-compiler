/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2022 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#pragma once

#include "Compiler/CodeGenPublic.h"
#include "Compiler/MetaDataUtilsWrapper.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/Pass.h>
#include <llvm/IR/InstVisitor.h>
#include "llvm/ADT/SetVector.h"
#include <llvm/Analysis/LoopInfo.h>
#include "common/LLVMWarningsPop.hpp"

namespace IGC
{
    llvm::FunctionPass* CreateEarlyOutPatternsPass();
    llvm::FunctionPass* CreateLowerFmaPass(bool respectFastMathFlags);
    llvm::FunctionPass* CreateHoistFMulInLoopPass();

    class CustomUnsafeOptPass : public llvm::FunctionPass, public llvm::InstVisitor<CustomUnsafeOptPass>
    {
    public:
        static char ID;

        CustomUnsafeOptPass();

        ~CustomUnsafeOptPass() {}

        virtual void getAnalysisUsage(llvm::AnalysisUsage& AU) const override
        {
            AU.setPreservesCFG();
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
        }

        virtual bool runOnFunction(llvm::Function& F) override;

        virtual llvm::StringRef getPassName() const override
        {
            return "Custom Unsafe Optimization Pass";
        }

        void visitInstruction(llvm::Instruction& I);
        void visitBinaryOperator(llvm::BinaryOperator& I);
        void visitFCmpInst(llvm::FCmpInst& FC);
        void visitSelectInst(llvm::SelectInst& I);
        void visitIntrinsicInst(llvm::IntrinsicInst& I);
        void visitFPToSIInst(llvm::FPToSIInst& I);

        bool visitBinaryOperatorPropNegate(llvm::BinaryOperator& I);
        bool visitBinaryOperatorNegateMultiply(llvm::BinaryOperator& I);
        bool visitBinaryOperatorTwoConstants(llvm::BinaryOperator& I);
        bool visitBinaryOperatorDivDivOp(llvm::BinaryOperator& I);
        bool visitBinaryOperatorDivRsq(llvm::BinaryOperator& I);
        bool visitBinaryOperatorAddDiv(llvm::BinaryOperator& I);
        bool visitBinaryOperatorFmulFaddPropagation(llvm::BinaryOperator& I);
        bool visitBinaryOperatorExtractCommonMultiplier(llvm::BinaryOperator& I);
        bool visitBinaryOperatorXor(llvm::BinaryOperator& I);
        bool removeCommonMultiplier(llvm::Value* I, llvm::Value* commonMultiplier);
        bool visitBinaryOperatorFmulToFmad(llvm::BinaryOperator& I);
        bool visitBinaryOperatorToFmad(llvm::BinaryOperator& I);
        bool visitBinaryOperatorAddSubOp(llvm::BinaryOperator& I);
        bool visitBinaryOperatorDivAddDiv(llvm::BinaryOperator& I);
        bool isFDiv(llvm::Value* I, llvm::Value*& numerator, llvm::Value*& denominator);
        bool possibleForFmadOpt(llvm::Instruction* inst);
        bool visitFCmpInstFCmpFAddOp(llvm::FCmpInst& FC);
        bool visitFCmpInstFCmpSelOp(llvm::FCmpInst& FC);
        bool visitFMulFCmpOp(llvm::FCmpInst& FC);
        bool visitExchangeCB(llvm::BinaryOperator& I);

        bool m_isChanged;
        bool m_disableReorderingOpt;
        IGC::CodeGenContext* m_ctx;
        IGCMD::MetaDataUtils* m_pMdUtils;

    private:
        inline llvm::BinaryOperator* copyIRFlags(
            llvm::BinaryOperator* newOp,
            llvm::Value* oldOp)
        {
            newOp->copyIRFlags(oldOp);

            llvm::DebugLoc dbg = ((llvm::Instruction*)oldOp)->getDebugLoc();
            newOp->setDebugLoc(dbg);

            return newOp;
        }

        void reassociateMulAdd(llvm::Function& F);

        void strengthReducePowOrExpLog(
            llvm::IntrinsicInst* intrin, llvm::Value* base, llvm::Value* exponent, bool isPow);

        void collectForErase(llvm::Instruction& I, unsigned int operandsDepth = 0);
        void eraseCollectedInst();

        llvm::SetVector<llvm::Instruction*> m_instToErase;
    };

} // namespace IGC
