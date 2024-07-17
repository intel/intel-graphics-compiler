/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

//===----------------------------------------------------------------------===//
//
// The purpose of this pass is replace instructions using halfs with
// corresponding float counterparts.
//
// All unnecessary conversions get cleaned up before code gen.
//
//===----------------------------------------------------------------------===//


#include "HalfPromotion.h"
#include "Compiler/IGCPassSupport.h"
#include "GenISAIntrinsics/GenIntrinsics.h"
#include "IGCIRBuilder.h"

#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Function.h>
#include "common/LLVMWarningsPop.hpp"


using namespace llvm;
using namespace IGC;

#define PASS_FLAG "half-promotion"
#define PASS_DESCRIPTION "Promotion of halfs to floats"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(HalfPromotion, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_END(HalfPromotion, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char HalfPromotion::ID = 0;

HalfPromotion::HalfPromotion() : FunctionPass(ID)
{
    initializeHalfPromotionPass(*PassRegistry::getPassRegistry());
}

bool HalfPromotion::runOnFunction(Function& F)
{
    visit(F);
    return m_changed;
}

void HalfPromotion::visitCallInst(llvm::CallInst& I)
{
    if (llvm::isa<GenIntrinsicInst>(I) && I.getType()->isHalfTy())
    {
        handleGenIntrinsic(llvm::cast<GenIntrinsicInst>(I));
    }
    else if (llvm::isa<llvm::IntrinsicInst>(I) && I.getType()->isHalfTy())
    {
        handleLLVMIntrinsic(llvm::cast<IntrinsicInst>(I));
    }
}

void IGC::HalfPromotion::handleLLVMIntrinsic(llvm::IntrinsicInst& I)
{
    Intrinsic::ID id = I.getIntrinsicID();
    if (id == Intrinsic::cos ||
        id == Intrinsic::sin ||
        id == Intrinsic::log2 ||
        id == Intrinsic::exp2 ||
        id == Intrinsic::sqrt ||
        id == Intrinsic::floor ||
        id == Intrinsic::ceil ||
        id == Intrinsic::fabs ||
        id == Intrinsic::pow ||
        id == Intrinsic::fma ||
        id == Intrinsic::maxnum ||
        id == Intrinsic::minnum)
    {
        Module* M = I.getParent()->getParent()->getParent();
        llvm::IGCIRBuilder<> builder(&I);
        std::vector<llvm::Value*> arguments;

        Function* pNewFunc = Intrinsic::getDeclaration(
            M,
            I.getIntrinsicID(),
            builder.getFloatTy());

        for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(&I); ++i)
        {
            if (I.getOperand(i)->getType()->isHalfTy())
            {
                Value* op = builder.CreateFPExt(I.getOperand(i), builder.getFloatTy());
                arguments.push_back(op);
            }
            else
            {
                arguments.push_back(I.getOperand(i));
            }
        }

        Value* f32Val = builder.CreateCall(
            pNewFunc,
            arguments);
        Value* f16Val = builder.CreateFPTrunc(f32Val, builder.getHalfTy());
        I.replaceAllUsesWith(f16Val);
        m_changed = true;
    }
}

void IGC::HalfPromotion::handleGenIntrinsic(llvm::GenIntrinsicInst& I)
{
    GenISAIntrinsic::ID id = I.getIntrinsicID();
    if (id == GenISAIntrinsic::GenISA_WaveAll ||
        id == GenISAIntrinsic::GenISA_WavePrefix ||
        id == GenISAIntrinsic::GenISA_WaveClustered ||
        id == GenISAIntrinsic::GenISA_WaveInterleave)
    {
        Module* M = I.getParent()->getParent()->getParent();
        llvm::IGCIRBuilder<> builder(&I);
        std::vector<llvm::Value*> arguments;

        Function* pNewFunc = GenISAIntrinsic::getDeclaration(
            M,
            I.getIntrinsicID(),
            builder.getFloatTy());

        for (unsigned i = 0; i < IGCLLVM::getNumArgOperands(&I); ++i)
        {
            if (I.getOperand(i)->getType()->isHalfTy())
            {
                Value* op = builder.CreateFPExt(I.getOperand(i), builder.getFloatTy());
                arguments.push_back(op);
            }
            else
            {
                arguments.push_back(I.getOperand(i));
            }
        }

        Value* f32Val = builder.CreateCall(
            pNewFunc,
            arguments);
        Value* f16Val = builder.CreateFPTrunc(f32Val, builder.getHalfTy());
        I.replaceAllUsesWith(f16Val);
        I.eraseFromParent();
        m_changed = true;
    }
}

void HalfPromotion::visitFCmp(llvm::FCmpInst& CmpI)
{
    if (CmpI.getOperand(0)->getType()->isHalfTy())
    {
        llvm::IGCIRBuilder<> builder(&CmpI);
        Value* op1 = builder.CreateFPExt(CmpI.getOperand(0), builder.getFloatTy());
        Value* op2 = builder.CreateFPExt(CmpI.getOperand(1), builder.getFloatTy());
        Value* newOp = builder.CreateFCmp(CmpI.getPredicate(), op1, op2);
        CmpI.replaceAllUsesWith(newOp);
        m_changed = true;
    }
}

void HalfPromotion::visitBinaryOperator(llvm::BinaryOperator& BI)
{
    if (BI.getType()->isHalfTy() &&
        (BI.getOpcode() == BinaryOperator::FAdd ||
            BI.getOpcode() == BinaryOperator::FSub ||
            BI.getOpcode() == BinaryOperator::FMul ||
            BI.getOpcode() == BinaryOperator::FDiv))
    {
        llvm::IGCIRBuilder<> builder(&BI);
        Value* op1 = builder.CreateFPExt(BI.getOperand(0), builder.getFloatTy());
        Value* op2 = builder.CreateFPExt(BI.getOperand(1), builder.getFloatTy());
        Value* newOp = builder.CreateBinOp(BI.getOpcode(), op1, op2);
        Value* f16Val = builder.CreateFPTrunc(newOp, builder.getHalfTy());
        BI.replaceAllUsesWith(f16Val);
        m_changed = true;
    }
}

/*

  What about casts like these?
  %162 = uitofp i32 %160 to half
  %163 = fpext half %162 to float
  %164 = fmul float %163, 1.600000e+01

  Is it safe to do this?
  %162 = uitofp i32 %160 to float
  %164 = fmul float %162, 1.600000e+01

*/

void HalfPromotion::visitCastInst(llvm::CastInst& CI)
{
    if (CI.getType()->isHalfTy() &&
        (CI.getOpcode() == CastInst::UIToFP ||
            CI.getOpcode() == CastInst::SIToFP))
    {
        llvm::IGCIRBuilder<> builder(&CI);
        Value* newOp = nullptr;
        if (CI.getOpcode() == CastInst::UIToFP)
        {
            newOp = builder.CreateUIToFP(CI.getOperand(0), builder.getFloatTy());
        }
        else
        {
            newOp = builder.CreateSIToFP(CI.getOperand(0), builder.getFloatTy());
        }
        Value* f16Val = builder.CreateFPTrunc(newOp, builder.getHalfTy());
        CI.replaceAllUsesWith(f16Val);
        m_changed = true;
    }
    else if (CI.getOperand(0)->getType()->isHalfTy() &&
        (CI.getOpcode() == CastInst::FPToUI ||
            CI.getOpcode() == CastInst::FPToSI))
    {
        llvm::IGCIRBuilder<> builder(&CI);
        Value* newOp = nullptr;
        Value* f32Val = builder.CreateFPExt(CI.getOperand(0), builder.getFloatTy());
        if (CI.getOpcode() == CastInst::FPToUI)
        {
            newOp = builder.CreateFPToUI(f32Val, CI.getType());
        }
        else
        {
            newOp = builder.CreateFPToSI(f32Val, CI.getType());
        }
        CI.replaceAllUsesWith(newOp);
        m_changed = true;
    }
}

void HalfPromotion::visitSelectInst(llvm::SelectInst& SI)
{
    if (SI.getTrueValue()->getType()->isHalfTy())
    {
        llvm::IGCIRBuilder<> builder(&SI);
        Value* opTrue = builder.CreateFPExt(SI.getTrueValue(), builder.getFloatTy());
        Value* opFalse = builder.CreateFPExt(SI.getFalseValue(), builder.getFloatTy());
        Value* newOp = builder.CreateSelect(SI.getCondition(), opTrue, opFalse);
        Value* f16Val = builder.CreateFPTrunc(newOp, builder.getHalfTy());
        SI.replaceAllUsesWith(f16Val);
        m_changed = true;
    }
}

void HalfPromotion::visitPHINode(llvm::PHINode& PHI)
{
    if (!PHI.getType()->isHalfTy())
    {
        return;
    }

    llvm::IGCIRBuilder<> builder(&PHI);
    llvm::PHINode* pNewPhi = llvm::PHINode::Create(builder.getFloatTy(), PHI.getNumIncomingValues(), "", &PHI);

    for (unsigned int i = 0; i < PHI.getNumIncomingValues(); ++i)
    {
        builder.SetInsertPoint(PHI.getIncomingBlock(i)->getTerminator());
        Value* phiFloatValue = builder.CreateFPExt(PHI.getIncomingValue(i), builder.getFloatTy());
        pNewPhi->addIncoming(phiFloatValue, PHI.getIncomingBlock(i));
    }

    builder.SetInsertPoint(PHI.getParent()->getFirstNonPHI());
    Value* f16Val = builder.CreateFPTrunc(pNewPhi, builder.getHalfTy());
    PHI.replaceAllUsesWith(f16Val);
    PHI.eraseFromParent();
    m_changed = true;
}
