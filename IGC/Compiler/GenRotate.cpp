/*========================== begin_copyright_notice ============================

Copyright (C) 2020-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#include "Compiler/GenRotate.hpp"
#include "Compiler/CISACodeGen/ShaderCodeGen.hpp"
#include "Compiler/CodeGenPublic.h"
#include "Compiler/IGCPassSupport.h"
#include "common/LLVMWarningsPush.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PatternMatch.h>
#include "common/LLVMWarningsPop.hpp"

using namespace llvm;
using namespace IGC;

namespace
{
    class GenRotate : public FunctionPass
    {
    public:
        static char ID; // Pass identification, replacement for typeid

        GenRotate() : FunctionPass(ID)
        {
            initializeGenRotatePass(*PassRegistry::getPassRegistry());
        }

        bool runOnFunction(Function& F) override;

        void getAnalysisUsage(AnalysisUsage& AU) const override
        {
            AU.addRequired<MetaDataUtilsWrapper>();
            AU.addRequired<CodeGenContextWrapper>();
            AU.setPreservesCFG();
        }

    private:
        CodeGenContext* m_Ctx = nullptr;
        Function* m_F = nullptr;
        const DataLayout* m_DL = nullptr;

        // Indicate supported integer width
        bool m_SupportInt8 = false;
        bool m_SupportInt16 = false;
        bool m_SupportInt32 = false;
        bool m_SupportInt64 = false;

        bool m_Changed = false;
        void matchRotate(Instruction* I);
    };
}  // namespace


#define PASS_FLAG "igc-genrotate"
#define PASS_DESCRIPTION "Generate rotate with llvm funnel shift intrinsic"
#define PASS_CFG_ONLY false
#define PASS_ANALYSIS false
IGC_INITIALIZE_PASS_BEGIN(GenRotate, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)
IGC_INITIALIZE_PASS_DEPENDENCY(MetaDataUtilsWrapper)
IGC_INITIALIZE_PASS_DEPENDENCY(CodeGenContextWrapper)
IGC_INITIALIZE_PASS_END(GenRotate, PASS_FLAG, PASS_DESCRIPTION, PASS_CFG_ONLY, PASS_ANALYSIS)

char GenRotate::ID = 0;

FunctionPass* IGC::createGenRotatePass()
{
    return new GenRotate();
}

bool GenRotate::runOnFunction(Function& F)
{
    CodeGenContextWrapper* pCtxWrapper = &getAnalysis<CodeGenContextWrapper>();
    m_Ctx = pCtxWrapper->getCodeGenContext();

    if (!m_Ctx->platform.supportRotateInstruction()) {
        return false;
    }

    m_SupportInt8 = false;
    m_SupportInt16 = true;
    m_SupportInt32 = true;
    m_SupportInt64 = false;
    m_SupportInt64 = m_Ctx->platform.supportQWRotateInstructions();

    m_Changed = false;
    for (auto BBI = F.begin(), BBE = F.end(); BBI != BBE; ++BBI)
    {
        // scan bottom-up
        BasicBlock* BB = &*BBI;
        for (auto II = BB->rbegin(), IE = BB->rend(); II != IE; /*empty */)
        {
            Instruction* I = &*II;

            // Increase II in case I is deleted in matchRotate()
            ++II;

            matchRotate(I);
        }
    }
    return m_Changed;
}

//
//  rol (V, amt) =  (V << amt) | ((unsigned(V) >> (N - amt))
//      where V is of type T (integer) with N bits, and amt is of integer type.
//
//  This function finds the following pattern, note that [insts] denotes that "insts" are optional.
//          [amt = and amt, N-1]
//          high = shl V0, amt
//          [amt0 = sub 0, amt  || amt0 = sub N, amt]
//          [amt0 = and amt0, N-1]
//          low = lshr V1, amt0
//          R = or high, low
//
//      case 0: [ likely, V is i32 or i64]
//          V = V0 (V0 == V1)
//
//      case 1:  [ likely V is i16 or i8]
//          V0 = sext V || zext V
//          V1 = zext V
//          Res = trunc R
//
//          Res's type == V's type
//
//  ror can be handled similarly. Note that
//    ror (x, amt) = ((unsigned)x >> amt) | ( x << (N - amt))
//                 = rol (x, N - amt);
//
void GenRotate::matchRotate(Instruction* I)
{
    // Note that if succesful, I is erased !
    using namespace llvm::PatternMatch;

    if (I->use_empty() || I->getType()->isVectorTy())
    {
        return;
    }

    Instruction* OrInst = nullptr;
    if (I->getOpcode() == Instruction::Trunc)
    {
        if (BinaryOperator* tmp = dyn_cast<BinaryOperator>(I->getOperand(0)))
        {
            if (tmp->getOpcode() == Instruction::Or)
            {
                OrInst = tmp;
            }
        }
    }
    else if (I->getOpcode() == Instruction::Or)
    {
        OrInst = I;
    }

    if (OrInst == nullptr)
    {
        return;
    }

    // Do rotate only if
    //   1) type is supported 16/32/64; and
    //   2) both operands are instructions.
    uint64_t typeWidth = I->getType()->getScalarSizeInBits();
    bool typeWidthSupported =
        ((m_SupportInt8 && typeWidth == 8) || (m_SupportInt16 && typeWidth == 16) ||
         (m_SupportInt32 && typeWidth == 32) || (m_SupportInt64 && typeWidth == 64));
    Instruction* LHS = dyn_cast<Instruction>(OrInst->getOperand(0));
    Instruction* RHS = dyn_cast<Instruction>(OrInst->getOperand(1));
    if (!LHS || !RHS || !typeWidthSupported)
    {
        return;
    }

    // Make adjustment so that LHS is shl.
    if (LHS->getOpcode() == Instruction::LShr)
    {
        Instruction* t = LHS;
        LHS = RHS;
        RHS = t;
    }
    if (LHS->getOpcode() != Instruction::Shl ||
        RHS->getOpcode() != Instruction::LShr)
    {
        return;
    }

    // first: find V
    Value* V0 = LHS->getOperand(0);
    Value* V1 = RHS->getOperand(0);
    Value* V = nullptr;
    if (I->getOpcode() == Instruction::Or)
    {
        if (V0 == V1)
        {
            V = V0;
        }
    }
    else
    {
        Value* X0 = nullptr, * X1 = nullptr;
        if ((match(V0, m_ZExt(m_Value(X0))) || match(V0, m_SExt(m_Value(X0)))) &&
            match(V1, m_ZExt(m_Value(X1))))
        {
            if (X0 == X1 && X0->getType()->getScalarSizeInBits() == typeWidth)
            {
                V = X0;
            }
        }
    }

    if (!V)
    {
        return;
    }

    // Second: find amt
    uint64_t typeMask = typeWidth - 1;
    Value* LAmt = LHS->getOperand(1);
    Value* RAmt = RHS->getOperand(1);
    ConstantInt* C_LAmt = dyn_cast<ConstantInt>(LAmt);
    ConstantInt* C_RAmt = dyn_cast<ConstantInt>(RAmt);
    Value* X0 = nullptr, * X1 = nullptr;
    Value* Amt = nullptr;
    bool isROL = true;
    if (C_LAmt || C_RAmt)
    {
        // If only one of shift-amounts is constant, it cannot be rotate.
        if (C_LAmt && C_RAmt)
        {
            // For shift amount that is beyond the typewidth, the result is
            // undefined. Here, we just use the LSB.
            uint64_t c0 = C_LAmt->getZExtValue() & typeMask;
            uint64_t c1 = C_RAmt->getZExtValue() & typeMask;
            if ((c0 + c1) == typeWidth)
            {
                Amt = LAmt;
                isROL = true;
            }
        }
    }
    else
    {
        if (match(RAmt, m_And(m_Sub(m_Zero(), m_Value(X1)), m_SpecificInt(typeMask))) ||
            match(RAmt, m_And(m_Sub(m_SpecificInt(typeWidth), m_Value(X1)), m_SpecificInt(typeMask))) ||
            match(RAmt, m_Sub(m_Zero(), m_Value(X1))) ||
            match(RAmt, m_Sub(m_SpecificInt(typeWidth), m_Value(X1))))
        {
            if (LAmt == X1 ||
                (match(LAmt, m_And(m_Value(X0), m_SpecificInt(typeMask))) && (X1 == X0)))
            {
                Amt = X1;
                isROL = true;
            }
        }
        if (!Amt &&
            (match(LAmt, m_And(m_Sub(m_Zero(), m_Value(X1)), m_SpecificInt(typeMask))) ||
             match(LAmt, m_And(m_Sub(m_SpecificInt(typeWidth), m_Value(X1)), m_SpecificInt(typeMask))) ||
             match(LAmt, m_Sub(m_Zero(), m_Value(X1))) ||
             match(LAmt, m_Sub(m_SpecificInt(typeWidth), m_Value(X1)))))
        {
            if (RAmt == X1 ||
                (match(RAmt, m_And(m_Value(X0), m_SpecificInt(typeMask))) && (X1 == X0)))
            {
                Amt = X1;
                isROL = false;
            }
        }

        if (Amt)
        {
            Value* X0, * X1, * X2 = nullptr;
            // 1) simple case: amt = typeWidth - X0;   use amt1 as shift amount.
            bool isReverse = match(Amt, m_Sub(m_SpecificInt(typeWidth), m_Value(X0)));

            // 2)   t = 16 - X0 | t = 0 - X0   ; for example,  t is i16/i8, etc
            //      t1 = t & 15
            //      amt = zext t1, i32
            isReverse = isReverse ||
                (match(Amt, m_ZExt(m_Value(X1))) &&
                 match(X1, m_And(m_Value(X2), m_SpecificInt(typeMask))) &&
                 (match(X2, m_Sub(m_SpecificInt(typeWidth), m_Value(X0))) ||
                  match(X2, m_Sub(m_Zero(), m_Value(X0)))));

            if (isReverse)
            {
                Amt = X0;
                isROL = !isROL;
            }
        }
    }

    if (!Amt)
    {
        return;
    }

    // Replace I with llvm.fshl or llvm.fshr
    IRBuilder<> Builder(I);
    // Create zext in case Amt has smaller width than V
    Amt = Builder.CreateZExt(Amt, V->getType());

    Intrinsic::ID rotateID = isROL ? Intrinsic::fshl : Intrinsic::fshr;
    Value* Args[3] = { V, V, Amt };
    Type* Ty = V->getType();
    CallInst* rotateCall = Builder.CreateIntrinsic(rotateID, Ty, Args, nullptr, "rotate");
    rotateCall->setDebugLoc(I->getDebugLoc());
    I->replaceAllUsesWith(rotateCall);
    I->eraseFromParent();

    m_Changed = true;
    return;
}
